/*
 * Note: this file originally auto-generated by mib2c using
 *        : mib2c.int_watch.conf,v 1.2 2002/07/17 14:41:53 dts12 Exp $
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <time.h>
#include <linux/version.h>
#include "lvs.h"
#include "libipvs.h"

static oid lvsVersion_oid[] = { 1,3,6,1,4,1,8225,4711,1, 0 };
static oid lvsNumServices_oid[] = { 1,3,6,1,4,1,8225,4711,2, 0 };
static oid lvsHashTableSize_oid[] = { 1,3,6,1,4,1,8225,4711,3, 0 };
static oid lvsTcpTimeOut_oid[] = { 1,3,6,1,4,1,8225,4711,4, 0 };
static oid lvsTcpTimeOutFin_oid[] = { 1,3,6,1,4,1,8225,4711,5, 0 };
static oid lvsUdpTimeOut_oid[] = { 1,3,6,1,4,1,8225,4711,6, 0 };
static oid lvsDaemonState_oid[] = { 1,3,6,1,4,1,8225,4711,7, 0 };
static oid lvsServiceTable_oid[] = {1,3,6,1,4,1,8225,4711,17};
static oid lvsRealTable_oid[] = {1,3,6,1,4,1,8225,4711,18};

struct Destination { 
	int svc_index;
	int dst_index;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	struct ip_vs_dest_entry* dest_entry;
#else
	struct ip_vs_dest_user* dest_entry;
#endif
	struct Destination* next; 
};

static struct ip_vs_get_services* ipvs_services;
static struct ip_vs_timeout_user* ipvs_timeout;
static struct ip_vs_daemon_user* ipvs_daemon;
static struct Destination* ipvs_destination;
static time_t last_setup;
static struct ip_vs_get_dests** sentry_base = NULL;

static
void setup_snmp_ipvs(void)
{
	int s, d;
	struct Destination* mydestprev = NULL;
	struct Destination* mydest = ipvs_destination;
	struct ip_vs_get_dests **sentry;
	
	time(&last_setup);
	if (ipvs_services) {
		free(ipvs_services);
		free(ipvs_timeout);
		free(ipvs_daemon);
	} else {
		ipvs_init();
	}
	if ((ipvs_services = ipvs_get_services())==NULL) {
		snmp_log(LOG_WARNING, "IPVS initialization failed");
		return;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	ipvs_timeout = ipvs_get_timeout();
#else
	ipvs_timeout = ipvs_get_timeouts();
#endif
	ipvs_daemon = ipvs_get_daemon();

	while (mydest) {
		mydestprev = mydest;
		mydest = mydest->next;
		SNMP_FREE(mydestprev);
	}
	mydestprev = NULL;
        ipvs_destination = NULL;

	/* NRC, 2010-05-18: Free old sentry structures... */
	sentry = sentry_base;
	if (sentry) {
		while (*sentry) {
			free(*sentry);
			sentry++;
		}
		free(sentry_base);
	}

	sentry_base = calloc(ipvs_services->num_services + 1, sizeof(struct ip_vs_get_dests *));
	for (s = 0; s<ipvs_services->num_services; s++) {
		sentry = sentry_base + s;
		*sentry = ipvs_get_dests(&ipvs_services->entrytable[s]);
		for (d = 0; d < (*sentry)->num_dests; d++) {
			mydest = SNMP_MALLOC_STRUCT(Destination);
			if (mydestprev==NULL) {
				ipvs_destination = mydest;
			} else {
				mydestprev->next = mydest;
			}
			mydest->dest_entry = &(*sentry)->entrytable[d];
			mydest->svc_index = s+1;
			mydest->dst_index = d+1;
			mydest->next = NULL;
			mydestprev = mydest;
		}
	}
}

static
int get_lvs_var(netsnmp_mib_handler* handler, netsnmp_handler_registration* reginfo,
                netsnmp_agent_request_info* reqinfo, netsnmp_request_info* requests)
{
	u_char string[SPRINT_MAX_LEN];
	int len;

	if (last_setup!=time(NULL))
		setup_snmp_ipvs();
	if (reqinfo->mode!=MODE_GET)
		return SNMP_ERR_GENERR;
	switch (reginfo->rootoid[8]) {
	    case 1:
		len = sprintf(string, "%d.%d.%d", NVERSION(ipvs_info.version));
		snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR, string, len);
		return SNMP_ERR_NOERROR;
	    case 2:
		snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, (u_char*) &ipvs_info.num_services, sizeof(ipvs_info.num_services));
		return SNMP_ERR_NOERROR;
	    case 3:
		snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, (u_char*) &ipvs_info.size, sizeof(ipvs_info.size));
		return SNMP_ERR_NOERROR;
	    case 4:
		if (ipvs_timeout)
			snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, (u_char*) &ipvs_timeout->tcp_timeout, sizeof(ipvs_timeout->tcp_timeout));
		return SNMP_ERR_NOERROR;
	    case 5:
		if (ipvs_timeout)
			snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, (u_char*) &ipvs_timeout->tcp_fin_timeout, sizeof(ipvs_timeout->tcp_fin_timeout));
		return SNMP_ERR_NOERROR;
	    case 6:
		if (ipvs_timeout)
			snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, (u_char*) &ipvs_timeout->udp_timeout, sizeof(ipvs_timeout->udp_timeout));
		return SNMP_ERR_NOERROR;
	    case 7:
		if (ipvs_daemon)
			snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, (u_char*) &ipvs_daemon->state, sizeof(ipvs_daemon->state));
		return SNMP_ERR_NOERROR;
	    default:
		return SNMP_ERR_GENERR;
	}
}

/** returns the first data point within the lvsServiceTable table data.

    Set the my_loop_context variable to the first data point structure
    of your choice (from which you can find the next one).  This could
    be anything from the first node in a linked list, to an integer
    pointer containing the beginning of an array variable.

    Set the my_data_context variable to something to be returned to
    you later (in your main lvsServiceTable_handler routine) that will provide
    you with the data to return in a given row.  This could be the
    same pointer as what my_loop_context is set to, or something
    different.

    The put_index_data variable contains a list of snmp variable
    bindings, one for each index in your table.  Set the values of
    each appropriately according to the data matching the first row
    and return the put_index_data variable at the end of the function.
*/
static
netsnmp_variable_list* lvsServiceTable_get_first_data_point(void **my_loop_context, void **my_data_context,
                          netsnmp_variable_list *put_index_data, netsnmp_iterator_info *mydata)
{
	int index = 0;
	netsnmp_variable_list* vptr;

	if (last_setup!=time(NULL))
		setup_snmp_ipvs();
	index++;
	if (ipvs_services==NULL || index>ipvs_services->num_services)
		return NULL;
	*my_loop_context = (void*)index;
	*my_data_context = (void*)index;
	vptr = put_index_data;
	snmp_set_var_value(vptr, (u_char *) &index, sizeof(index));
	vptr = vptr->next_variable;
	return put_index_data;
}

/** functionally the same as lvsServiceTable_get_first_data_point, but
   my_loop_context has already been set to a previous value and should
   be updated to the next in the list.  For example, if it was a
   linked list, you might want to cast it and the return
   my_loop_context->next.  The my_data_context pointer should be set
   to something you need later and the indexes in put_index_data
   updated again. */
static
netsnmp_variable_list* lvsServiceTable_get_next_data_point(void** my_loop_context, void** my_data_context,
                         netsnmp_variable_list* put_index_data, netsnmp_iterator_info* mydata)
{
	int index = (int)*my_loop_context;
	netsnmp_variable_list *vptr;

	index++;
	if (ipvs_services==NULL || index>ipvs_services->num_services)
		return NULL;
	*my_loop_context = (void*)index;
	*my_data_context = (void*)index;
	vptr = put_index_data;
	snmp_set_var_value(vptr, (u_char*) &index, sizeof(index));
	vptr = vptr->next_variable;
	return put_index_data;
}

/** handles requests for the lvsServiceTable table, if anything else needs to be done */
static
int lvsServiceTable_handler(netsnmp_mib_handler* handler, netsnmp_handler_registration* reginfo,
    netsnmp_agent_request_info* reqinfo, netsnmp_request_info* requests)
{
	netsnmp_request_info* request;
	netsnmp_table_request_info* table_info;
	netsnmp_variable_list* var;
	int svcindex;
	unsigned int tmp;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	struct ip_vs_service_entry* entrytable;
#else
	struct ip_vs_service_user* entrytable;
#endif
	struct ip_vs_stats_user* stats;

	for (request = requests; request; request = request->next) {
		var = request->requestvb;
		if (request->processed!=0)
			continue;
		/* the following extracts the my_data_context pointer set in
		the loop functions above.  You can then use the results to
		help return data for the columns of the lvsServiceTable table in question */
		svcindex = (int) netsnmp_extract_iterator_context(request);
		if (svcindex==0) {
			if (reqinfo->mode==MODE_GET)
				netsnmp_set_request_error(reqinfo, requests, SNMP_NOSUCHINSTANCE);
			return SNMP_ERR_NOERROR;
		}
		entrytable = &ipvs_services->entrytable[svcindex-1];
		stats = &entrytable->stats;
		table_info = netsnmp_extract_table_info(request);
	        if (table_info==NULL)
        		continue;
		if (reqinfo->mode==MODE_GET) {
			switch(table_info->colnum) {
			    case COLUMN_LVSSERVICENUMBER:
				snmp_set_var_typed_value(var, ASN_INTEGER, (u_char*)&svcindex, sizeof(int));
				break;
			    case COLUMN_LVSSERVICESCHEDTYPE:
				snmp_set_var_typed_value(var, ASN_OCTET_STR, entrytable->sched_name, strlen(entrytable->sched_name));
				break;
			    case COLUMN_LVSSERVICEPROTO:
				tmp = entrytable->protocol;
				snmp_set_var_typed_value(var, ASN_INTEGER, (u_char*)&tmp, sizeof(int));
				break;
			    case COLUMN_LVSSERVICEADDR:
				if (entrytable->af == AF_INET) {
					snmp_set_var_typed_value(var, ASN_IPADDRESS, (u_char*) &entrytable->addr.in, sizeof(entrytable->addr.in));
				}
				else if (entrytable->af == AF_INET6) {
					snmp_set_var_typed_value(var, ASN_IPADDRESS, (u_char*) &entrytable->addr.in6, sizeof(entrytable->addr.in6));
				}
				break;
			    case COLUMN_LVSSERVICEPORT:
				tmp = htons(entrytable->port);
				snmp_set_var_typed_value(var, ASN_INTEGER, (u_char*) &tmp, sizeof(int));
				break;
			    case COLUMN_LVSSERVICEFWMARK:
				snmp_set_var_typed_value(var, ASN_INTEGER, (u_char*) &entrytable->fwmark, sizeof(int));
				break;
			    case COLUMN_LVSSERVICEPERSISTTIMEOUT:
				snmp_set_var_typed_value(var, ASN_INTEGER, (u_char*) &entrytable->timeout, sizeof(int));
				break;
			    case COLUMN_LVSSERVICEPERSISTNETMASK:
				snmp_set_var_typed_value(var, ASN_IPADDRESS, (u_char*) &entrytable->netmask, sizeof(entrytable->netmask));
				break;
			    case COLUMN_LVSSERVICENUMDESTS:
				snmp_set_var_typed_value(var, ASN_INTEGER, (u_char *) &entrytable->num_dests, sizeof(entrytable->num_dests));
				break;
			    case COLUMN_LVSSERVICESTATSCONNS:
				snmp_set_var_typed_value(var, ASN_COUNTER, (u_char *) &stats->conns, sizeof(stats->conns));
				break;
			    case COLUMN_LVSSERVICESTATSINPKTS:
				snmp_set_var_typed_value(var, ASN_COUNTER, (u_char *) &stats->inpkts, sizeof(stats->inpkts));
				break;
			    case COLUMN_LVSSERVICESTATSOUTPKTS:
				snmp_set_var_typed_value(var, ASN_COUNTER, (u_char *) &stats->outpkts, sizeof(stats->outpkts));
				break;
			    case COLUMN_LVSSERVICESTATSINBYTES:
				{
					struct counter64 c64 = {
						.high = (stats->inbytes >> 32) & 0xffffffff,
						.low  = (stats->inbytes      ) & 0xffffffff
					};
					snmp_set_var_typed_value(var, ASN_COUNTER64, (u_char *) &c64, sizeof(c64));
					break;
				}
			    case COLUMN_LVSSERVICESTATSOUTBYTES:
				{
					struct counter64 c64 = {
						.high = (stats->outbytes >> 32) & 0xffffffff,
						.low  = (stats->outbytes      ) & 0xffffffff
					};
					snmp_set_var_typed_value(var, ASN_COUNTER64, (u_char *)&c64, sizeof(c64));
					break;
				}
			    case COLUMN_LVSSERVICERATECPS:
				snmp_set_var_typed_value(var, ASN_GAUGE, (u_char *) &stats->cps, sizeof(stats->cps));
				break;
			    case COLUMN_LVSSERVICERATEINPPS:
				snmp_set_var_typed_value(var, ASN_GAUGE, (u_char *) &stats->inpps, sizeof(stats->inpps));
				break;
			    case COLUMN_LVSSERVICERATEOUTPPS:
				snmp_set_var_typed_value(var, ASN_GAUGE, (u_char *) &stats->outpps, sizeof(stats->outpps));
				break;
			    case COLUMN_LVSSERVICERATEINBPS:
				snmp_set_var_typed_value(var, ASN_GAUGE, (u_char *) &stats->inbps, sizeof(stats->inbps));
				break;
			    case COLUMN_LVSSERVICERATEOUTBPS:
				snmp_set_var_typed_value(var, ASN_GAUGE, (u_char *) &stats->outbps, sizeof(stats->outbps));
				break;
			    default:
				/* We shouldn't get here */
				snmp_log(LOG_ERR, "problem encountered in lvsServiceTable_handler: unknown column\n");
			}
		} else {
			snmp_log(LOG_ERR, "problem encountered in lvsServiceTable_handler: unsupported mode\n");
		}
	}
	return SNMP_ERR_NOERROR;
}

void
initialize_table_lvsServiceTable(void)
{
	netsnmp_table_registration_info *table_info;
	netsnmp_handler_registration *my_handler;
	netsnmp_iterator_info *iinfo;

	table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
	iinfo = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
	my_handler = netsnmp_create_handler_registration("lvsServiceTable",
			     lvsServiceTable_handler,
			     lvsServiceTable_oid,
			     OID_LENGTH(lvsServiceTable_oid),
			     HANDLER_CAN_RWRITE);

	if (!my_handler || !table_info || !iinfo)
		return; /* mallocs failed */
	netsnmp_table_helper_add_indexes(table_info, ASN_INTEGER, 0);
	table_info->min_column = 1;
	table_info->max_column = 19;
	iinfo->get_first_data_point = lvsServiceTable_get_first_data_point;
	iinfo->get_next_data_point = lvsServiceTable_get_next_data_point;
	iinfo->table_reginfo = table_info;
	netsnmp_register_table_iterator(my_handler, iinfo);
}

static
netsnmp_variable_list* lvsRealTable_get_first_data_point(void **my_loop_context, void **my_data_context,
                          netsnmp_variable_list *put_index_data, netsnmp_iterator_info *mydata)
{
	netsnmp_variable_list* vptr;

	if (last_setup!=time(NULL))
		setup_snmp_ipvs();
	if (ipvs_destination==NULL)
		return NULL;
	*my_loop_context = (void*)ipvs_destination;
	*my_data_context = (void*)ipvs_destination;
	vptr = put_index_data;
	snmp_set_var_value(vptr, (u_char *) &ipvs_destination->svc_index, sizeof(ipvs_destination->svc_index));
	vptr = vptr->next_variable;
	snmp_set_var_value(vptr, (u_char*) &ipvs_destination->dst_index, sizeof(ipvs_destination->dst_index));
	vptr = vptr->next_variable;
	return put_index_data;
}

static
netsnmp_variable_list* lvsRealTable_get_next_data_point(void **my_loop_context, void **my_data_context,
                         netsnmp_variable_list *put_index_data, netsnmp_iterator_info *mydata)
{
	netsnmp_variable_list *vptr;
	struct Destination* destnext;

	destnext = ((struct Destination*)*my_loop_context)->next;
	if (destnext==NULL)
		return NULL;
	*my_loop_context = (void*)destnext;
	*my_data_context =  (void*)destnext;
	vptr = put_index_data;
	snmp_set_var_value(vptr, (u_char *) &destnext->svc_index, sizeof(destnext->svc_index));
	vptr = vptr->next_variable;
	snmp_set_var_value(vptr, (u_char *) &destnext->dst_index, sizeof(destnext->dst_index));
	vptr = vptr->next_variable;
	return put_index_data;
}

static
int lvsRealTable_handler(netsnmp_mib_handler* handler, netsnmp_handler_registration* reginfo,
    netsnmp_agent_request_info* reqinfo, netsnmp_request_info* requests)
{
	netsnmp_request_info *request;
	netsnmp_table_request_info *table_info;
	netsnmp_variable_list *var;
	struct Destination* mydest;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	struct ip_vs_dest_entry* destentry;
#else
	struct ip_vs_dest_user* destentry;
#endif
	struct ip_vs_stats_user* stats;
	int tmp;

	for (request = requests; request; request = request->next) {
		var = request->requestvb;
		if (request->processed != 0)
			continue;
		mydest = (struct Destination*) netsnmp_extract_iterator_context(request);
		if (mydest==NULL) {
			if (reqinfo->mode==MODE_GET)
				netsnmp_set_request_error(reqinfo, request, SNMP_NOSUCHINSTANCE);
			continue;
		}
		destentry = mydest->dest_entry;
		stats = &destentry->stats;
		table_info = netsnmp_extract_table_info(request);
		if (table_info==NULL)
			continue;
		if (reqinfo->mode==MODE_GET) {
			switch(table_info->colnum) {
			    case COLUMN_LVSREALSERVICENUMBER:
				snmp_set_var_typed_value(var, ASN_INTEGER, (u_char*) &mydest->svc_index, sizeof(mydest->svc_index));
				break;
			    case COLUMN_LVSREALSERVERNUMBER:
				snmp_set_var_typed_value(var, ASN_INTEGER, (u_char*) &mydest->dst_index, sizeof(mydest->dst_index));
				break;
			    case COLUMN_LVSREALSERVERADDR:
				if (destentry->af == AF_INET) {
					snmp_set_var_typed_value(var, ASN_IPADDRESS, (u_char*) &destentry->addr.in, sizeof(destentry->addr.in));
				}
				else if (destentry->af == AF_INET6) {
					snmp_set_var_typed_value(var, ASN_IPADDRESS, (u_char*) &destentry->addr.in6, sizeof(destentry->addr.in6));
				}
				break;
			    case COLUMN_LVSREALSERVERPORT:
				tmp = htons(destentry->port);
				snmp_set_var_typed_value(var, ASN_INTEGER, (u_char*) &tmp, sizeof(tmp));
				break;
			    case COLUMN_LVSREALSERVERFLAGS:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
				snmp_set_var_typed_value(var, ASN_INTEGER, (u_char*) &destentry->conn_flags, sizeof(destentry->conn_flags));
#else
				snmp_set_var_typed_value(var, ASN_INTEGER, (u_char*) &destentry->flags, sizeof(destentry->flags));
#endif
				break;
			    case COLUMN_LVSREALSERVERWEIGHT:
				snmp_set_var_typed_value(var, ASN_INTEGER, (u_char*) &destentry->weight, sizeof(destentry->weight));
				break;
			    case COLUMN_LVSREALSTATSCONNS:
				snmp_set_var_typed_value(var, ASN_COUNTER, (u_char*) &stats->conns, sizeof(stats->conns));
				break;
			    case COLUMN_LVSREALSTATSINPKTS:
				snmp_set_var_typed_value(var, ASN_COUNTER, (u_char*) &stats->inpkts, sizeof(stats->inpkts));
				break;
			    case COLUMN_LVSREALSTATSOUTPKTS:
				snmp_set_var_typed_value(var, ASN_COUNTER, (u_char *) &stats->outpkts, sizeof(stats->outpkts) );
				break;
			    case COLUMN_LVSREALSTATSINBYTES:
				{
					struct counter64 c64 = {
						.high = (stats->inbytes >> 32) & 0xffffffff,
						.low  = (stats->inbytes      ) & 0xffffffff
					};
					snmp_set_var_typed_value(var, ASN_COUNTER64, (u_char *) &c64, sizeof(c64));
					break;
				}
			    case COLUMN_LVSREALSTATSOUTBYTES:
				{
					struct counter64 c64 = {
						.high = (stats->outbytes >> 32) & 0xffffffff,
						.low  = (stats->outbytes      ) & 0xffffffff
					};
					snmp_set_var_typed_value(var, ASN_COUNTER64, (u_char *) &c64, sizeof(c64));
					break;
				}
			    case COLUMN_LVSREALRATECPS:
				snmp_set_var_typed_value(var, ASN_GAUGE, (u_char *) &stats->cps, sizeof(stats->cps));
				break;
			    case COLUMN_LVSREALRATEINPPS:
				snmp_set_var_typed_value(var, ASN_GAUGE, (u_char *) &stats->inpps, sizeof(stats->inpps));
				break;
			    case COLUMN_LVSREALRATEOUTPPS:
				snmp_set_var_typed_value(var, ASN_GAUGE, (u_char *) &stats->outpps, sizeof(stats->outpps));
				break;
			    case COLUMN_LVSREALRATEINBPS:
				snmp_set_var_typed_value(var, ASN_GAUGE, (u_char *) &stats->inbps, sizeof(stats->inbps));
				break;
			    case COLUMN_LVSREALRATEOUTBPS:
				snmp_set_var_typed_value(var, ASN_GAUGE, (u_char *) &stats->outbps, sizeof(stats->outbps));
				break;
			    default:
				/* We shouldn't get here */
				snmp_log(LOG_ERR, "problem encountered in lvsRealTable_handler: unknown column\n");
			}
		} else {
			snmp_log(LOG_ERR, "problem encountered in lvsRealTable_handler: unsupported mode\n");
		}
	}
	return SNMP_ERR_NOERROR;
}

void initialize_table_lvsRealTable(void)
{
	netsnmp_table_registration_info *table_info;
	netsnmp_handler_registration *my_handler;
	netsnmp_iterator_info *iinfo;

	table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
	iinfo = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
	my_handler = netsnmp_create_handler_registration("lvsRealTable",
					     lvsRealTable_handler,
					     lvsRealTable_oid,
					     OID_LENGTH(lvsRealTable_oid),
					     HANDLER_CAN_RONLY);
	if (!my_handler || !table_info || !iinfo)
		return; /* mallocs failed */
	netsnmp_table_helper_add_indexes(table_info, ASN_INTEGER, ASN_INTEGER, 0); 
	table_info->min_column = 1;
	table_info->max_column = 16;
	iinfo->get_first_data_point = lvsRealTable_get_first_data_point;
	iinfo->get_next_data_point = lvsRealTable_get_next_data_point;
	iinfo->table_reginfo = table_info;
	netsnmp_register_table_iterator(my_handler, iinfo);
}


void init_lvs(void)
{
	snmp_log(LOG_INFO, "IPVS initialization for ");
	netsnmp_register_read_only_instance(netsnmp_create_handler_registration("lvsVersion", get_lvs_var, lvsVersion_oid, OID_LENGTH(lvsVersion_oid), HANDLER_CAN_RONLY));
	netsnmp_register_read_only_instance(netsnmp_create_handler_registration("lvsNumServices", get_lvs_var, lvsNumServices_oid, OID_LENGTH(lvsNumServices_oid), HANDLER_CAN_RONLY));
	netsnmp_register_read_only_instance(netsnmp_create_handler_registration("lvsHashTableSize", get_lvs_var, lvsHashTableSize_oid, OID_LENGTH(lvsHashTableSize_oid), HANDLER_CAN_RONLY));
	netsnmp_register_read_only_instance(netsnmp_create_handler_registration("lvsTcpTimeOut", get_lvs_var, lvsTcpTimeOut_oid, OID_LENGTH(lvsTcpTimeOut_oid), HANDLER_CAN_RONLY));
	netsnmp_register_read_only_instance(netsnmp_create_handler_registration("lvsTcpTimeOutFin", get_lvs_var, lvsTcpTimeOutFin_oid, OID_LENGTH(lvsTcpTimeOutFin_oid), HANDLER_CAN_RONLY));
	netsnmp_register_read_only_instance(netsnmp_create_handler_registration("lvsUdpTimeOut", get_lvs_var, lvsUdpTimeOut_oid, OID_LENGTH(lvsUdpTimeOut_oid), HANDLER_CAN_RONLY));
	netsnmp_register_read_only_instance(netsnmp_create_handler_registration("lvsDaemonState", get_lvs_var, lvsDaemonState_oid, OID_LENGTH(lvsDaemonState_oid), HANDLER_CAN_RONLY));
	initialize_table_lvsServiceTable();
	initialize_table_lvsRealTable();
}

