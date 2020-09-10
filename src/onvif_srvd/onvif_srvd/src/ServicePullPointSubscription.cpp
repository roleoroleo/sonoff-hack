/*
 --------------------------------------------------------------------------
 ServicePullPointSubscription.cpp
.
 Implementation of functions (methods) for the service:
 ONVIF event.wsdl server side
-----------------------------------------------------------------------------
*/


#include <sstream>
#include "soapPullPointSubscriptionBindingService.h"
#include "ServiceContext.h"
#include "smacros.h"
#include <sys/sysinfo.h>
#include <sqlite3.h>

#define IPCSYS_DB "/mnt/mtd/db/ipcsys.db"

/*
    struct Namespace namespaces[] =
    {   // {"ns-prefix", "ns-name"}
        { "SOAP-ENV", "http://www.w3.org/2003/05/soap-envelope" }, // MUST be first
        { "SOAP-ENC", "http://www.w3.org/2003/05/soap-encoding" },
        { "xsi", "http://www.w3.org/2001/XMLSchema-instance" },
        { "xsd", "http://www.w3.org/2001/XMLSchema" },
        { "chan", "http://schemas.microsoft.com/ws/2005/02/duplex" },
        { "wsa5", "http://www.w3.org/2005/08/addressing" },
        { "wsrfr", "http://docs.oasis-open.org/wsrf/r-2" },
        { "wsa", "http://schemas.xmlsoap.org/ws/2004/08/addressing"},
        { "wsrfbf", "http://docs.oasis-open.org/wsrf/bf-2" },
        { "wsnt", "http://docs.oasis-open.org/wsn/b-2" },
        { "xmime", "http://tempuri.org/xmime.xsd" },
        { "xop", "http://www.w3.org/2004/08/xop/include" },
        { "tt", "http://www.onvif.org/ver10/schema" },
        { "tns1", "http://www.onvif.org/ver10/topics" },
        { "wstop", "http://docs.oasis-open.org/wsn/t-1" },
        { "tds", "http://www.onvif.org/ver10/device/wsdl" },
        { "tev", "http://www.onvif.org/ver10/events/wsdl" },
        { "tptz", "http://www.onvif.org/ver20/ptz/wsdl" },
        { "trt", "http://www.onvif.org/ver10/media/wsdl" },
        { "c14n", "http://www.w3.org/2001/10/xml-exc-c14n#" },
        { "ds", "http://www.w3.org/2000/09/xmldsig#" },
        { "saml1", "urn:oasis:names:tc:SAML:1.0:assertion" },
        { "saml2", "urn:oasis:names:tc:SAML:2.0:assertion" },
        { "wsu", "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd" },
        { "xenc", "http://www.w3.org/2001/04/xmlenc#" },
        { "wsc", "http://docs.oasis-open.org/ws-sx/ws-secureconversation/200512" },
        { "wsse", "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd" },
        {NULL, NULL} // end of table
    };

    soap_set_namespaces(soap, namespaces_tns1);
*/

int PullPointSubscriptionBindingService::PullMessages(_tev__PullMessages *tev__PullMessages, _tev__PullMessagesResponse &tev__PullMessagesResponse)
{
    bool pull_motion_alarm = false;
    bool pull_processor_usage = false;
    bool pull_last_reboot = false;
    time_t now_t;
    char iso_ct[32];
    char iso_lb[32];
    char load[32];
    int message_limit;
    std::vector<Subscription>::iterator it;

    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;

    // Fix response message
    soap->header->wsa5__Action = (char *) "http://www.onvif.org/ver10/events/wsdl/PullPointSubscription/PullMessagesResponse";

    // Get subscription endpoint from request header
    std::string subscriptionEndpoint = soap->header->wsa5__To;

    // Get Timeout and decrease CPU load
    LONG64 to = tev__PullMessages->Timeout;
    int ito = (int) (to / 1000);
    if (ito > 1) sleep(1);

    // Get Message Limit
    message_limit = tev__PullMessages->MessageLimit;

    // Find subscription
    bool sub_found = false;
    for(it = std::begin(*(ctx->get_subscriptions())); it != std::end(*(ctx->get_subscriptions())); ++it) {
        if (it->get_address().compare(subscriptionEndpoint) == 0) {
            time_t ct = time(NULL);
            if (it->get_termination_time() < ct) {
                ctx->get_subscriptions()->erase(it);
            } else {
                sub_found = true;
            }
            break;
        }
    }

    if (!sub_found) {
        return SOAP_OK;
    }

    time(&now_t);

    // MotionAlarm
    // Get last alarm from db in string format "2020-09-06 15:14:15"
    sqlite3 *dbc = NULL;
    int ret;
    sqlite3_stmt *stmt = NULL;
    char buffer[1024];
    std::string alarm;
    char iso_utc[32] = "1970-01-01T00:00:00Z";

    ret = sqlite3_open_v2(IPCSYS_DB, &dbc, SQLITE_OPEN_READONLY, NULL);
    if (ret == SQLITE_OK) {
        sprintf (buffer, "select max(c_alarm_time), c_alarm_context from t_alarm_log;");
        ret = sqlite3_prepare_v2(dbc, buffer, -1, &stmt, NULL);
        if (ret == SQLITE_OK) {
            ret = sqlite3_step(stmt);
            if (ret == SQLITE_ROW) {
                alarm = (char *) sqlite3_column_text(stmt, 0);
            }

            sqlite3_reset(stmt);
            sqlite3_finalize(stmt);

            // Check syntax
            if ((alarm.size() == 19) && (alarm.at(4) == '-') && (alarm.at(7) == '-') && (alarm.at(10) == ' ') && (alarm.at(13) == ':') && (alarm.at(16) == ':')) {

                struct tm *timeinfo, *timeinfo_utc;
                int itmp;
                time_t alarm_t;

                timeinfo = localtime(&now_t);
                // Year
                itmp = atoi(alarm.substr(0, 4).c_str());
                if (itmp != 0) {
                    timeinfo->tm_year = itmp - 1900;
                    // Month
                    itmp = atoi(alarm.substr(5, 2).c_str());
                    if (itmp != 0) {
                        timeinfo->tm_mon = itmp - 1;
                        // Other
                        timeinfo->tm_mday = atoi(alarm.substr(8, 2).c_str());
                        timeinfo->tm_hour = atoi(alarm.substr(11, 2).c_str());
                        timeinfo->tm_min = atoi(alarm.substr(14, 2).c_str());
                        timeinfo->tm_sec = atoi(alarm.substr(17, 2).c_str());
                        alarm_t = mktime(timeinfo);

                        // If there is a new row in the alarm table, start new alarm
                        // If there isn't a new row in the alarm table and the alarm state is true, wait 10 seconds and stop the alarm
                        if ((alarm_t > ctx->get_last_motion_alarm()) && (ctx->get_last_motion_alarm() == 0)) {
                            ctx->set_last_motion_alarm_state(false);
                            ctx->set_last_motion_alarm(alarm_t);
                            pull_motion_alarm = false;
                        } else if (alarm_t > ctx->get_last_motion_alarm()) {
                            ctx->set_last_motion_alarm_state(true);
                            ctx->set_last_motion_alarm(alarm_t);
                            pull_motion_alarm = true;
                        } else if ((alarm_t == ctx->get_last_motion_alarm()) && (ctx->get_last_motion_alarm_state() == true)) {
                            if (now_t - alarm_t > 10) {
                                ctx->set_last_motion_alarm_state(false);
                                alarm_t += 10;
                                pull_motion_alarm = true;
                            } else {
                                pull_motion_alarm = false;
                            }
                        } else if ((alarm_t == ctx->get_last_motion_alarm()) && (ctx->get_last_motion_alarm_state() == false)) {
                            pull_motion_alarm = false;
                        }
                        timeinfo_utc = gmtime(&alarm_t);
                        sprintf(iso_utc, "%04d-%02d-%02dT%02d:%02d:%02dZ", timeinfo_utc->tm_year + 1900, timeinfo_utc->tm_mon + 1, timeinfo_utc->tm_mday, timeinfo_utc->tm_hour, timeinfo_utc->tm_min, timeinfo_utc->tm_sec);
                    }
                }
            }
        }
    }
    if (dbc) {
        sqlite3_close_v2(dbc);
    }

    // ProcessorUsage and LastReboot
    if ((now_t % 10) == 0) {
        struct sysinfo s_info;
        time_t ct_t;
        struct tm *timeinfoct;
        struct tm *timeinfolb;
        float f_load;
        int error;

        time(&ct_t);
        timeinfoct = gmtime(&ct_t);
        sprintf(iso_ct, "%04d-%02d-%02dT%02d:%02d:%02dZ", timeinfoct->tm_year + 1900, timeinfoct->tm_mon + 1, timeinfoct->tm_mday, timeinfoct->tm_hour, timeinfoct->tm_min, timeinfoct->tm_sec);

        error = sysinfo(&s_info);
        if (error == 0) {
            ct_t -= s_info.uptime;

            timeinfolb = gmtime(&ct_t);
            sprintf(iso_lb, "%04d-%02d-%02dT%02d:%02d:%02dZ", timeinfolb->tm_year + 1900, timeinfolb->tm_mon + 1, timeinfolb->tm_mday, timeinfolb->tm_hour, timeinfolb->tm_min, timeinfolb->tm_sec);
            f_load = 1.f / (1 << SI_LOAD_SHIFT);
            sprintf(load, "%.2f", s_info.loads[0] * f_load);

            pull_processor_usage = true;
            pull_last_reboot = true;
        }
    }


    if ((!pull_motion_alarm) && (!pull_processor_usage) && (!pull_last_reboot)) {
        return SOAP_OK;
    }

    // Set CurrentTime and TerminationTime
    time_t ct = time(NULL);
    tev__PullMessagesResponse.CurrentTime = ct;
    tev__PullMessagesResponse.TerminationTime = ct + it->get_initial_termination_time();

    if ((message_limit >= 1) && (pull_motion_alarm)) {

        // MotionAlarm
        // Create NotificationMessage starting from bottom
        soap_dom_element *sis = new soap_dom_element(soap, NULL, "tt:SimpleItem");
        sis->add(new soap_dom_attribute(soap, NULL, "Name", "VideoSourceToken"));
        sis->add(new soap_dom_attribute(soap, NULL, "Value", "MA0"));
        soap_dom_element *s = new soap_dom_element(soap, NULL, "tt:Source");
        s->add(*sis);

        soap_dom_element *sid = new soap_dom_element(soap, NULL, "tt:SimpleItem");
        sid->add(new soap_dom_attribute(soap, NULL, "Name", "State"));
        if (ctx->get_last_motion_alarm_state()) {
            sid->add(new soap_dom_attribute(soap, NULL, "Value", "True"));
        } else {
            sid->add(new soap_dom_attribute(soap, NULL, "Value", "False"));
        }
        soap_dom_element *d = new soap_dom_element(soap, NULL, "tt:Data");
        d->add(*sid);

        // tt:message
        soap_dom_element *m = new soap_dom_element(soap, NULL, "tt:Message");
        m->add(new soap_dom_attribute(soap, NULL, "UtcTime", iso_utc));
        m->add(*s);
        m->add(*d);

        // wsnt:Message
        _wsnt__NotificationMessageHolderType_Message *nmhtm = soap_new__wsnt__NotificationMessageHolderType_Message(soap);
        nmhtm->__any = *m;

        // wsnt:Topic
        wsnt__TopicExpressionType *tet = soap_new_wsnt__TopicExpressionType(soap);
        tet->Dialect = "http://www.onvif.org/ver10/tev/topicExpression/ConcreteSet";
        tet->__any = "tns1:VideoSource/MotionAlarm";

        // wsnt:SubscriptionReference
        wsa5__EndpointReferenceType *ert = soap_new_wsa5__EndpointReferenceType(soap);
        ert->Address = (char *) strdup(subscriptionEndpoint.c_str());

        // Add Topic, Message and SubscriptionReference to NotificationMessage
        wsnt__NotificationMessageHolderType *nmht = soap_new_wsnt__NotificationMessageHolderType(soap);
        nmht->Topic = tet;
        nmht->Message = *nmhtm;
        nmht->SubscriptionReference = ert;

        // Add NotificationMessage to response
        tev__PullMessagesResponse.wsnt__NotificationMessage.push_back(nmht);

    }

    message_limit--;

    if ((message_limit >= 1) && (pull_processor_usage)) {

        // ProcessorUsage
        // Create NotificationMessage starting from bottom
        soap_dom_element *sis = new soap_dom_element(soap, NULL, "tt:SimpleItem");
        sis->add(new soap_dom_attribute(soap, NULL, "Name", "Token"));
        sis->add(new soap_dom_attribute(soap, NULL, "Value", "PU0"));
        soap_dom_element *s = new soap_dom_element(soap, NULL, "tt:Source");
        s->add(*sis);

        soap_dom_element *sid = new soap_dom_element(soap, NULL, "tt:SimpleItem");
        sid->add(new soap_dom_attribute(soap, NULL, "Name", "Value"));
        sid->add(new soap_dom_attribute(soap, NULL, "Value", load));
        soap_dom_element *d = new soap_dom_element(soap, NULL, "tt:Data");
        d->add(*sid);

        // tt:message
        soap_dom_element *m = new soap_dom_element(soap, NULL, "tt:Message");
        m->add(new soap_dom_attribute(soap, NULL, "UtcTime", iso_ct));
        m->add(*s);
        m->add(*d);

        // wsnt:Message
        _wsnt__NotificationMessageHolderType_Message *nmhtm = soap_new__wsnt__NotificationMessageHolderType_Message(soap);
        nmhtm->__any = *m;

        // wsnt:Topic
        wsnt__TopicExpressionType *tet = soap_new_wsnt__TopicExpressionType(soap);
        tet->Dialect = "http://www.onvif.org/ver10/tev/topicExpression/ConcreteSet";
        tet->__any = "tns1:Monitoring/ProcessorUsage";

        // wsnt:SubscriptionReference
        wsa5__EndpointReferenceType *ert = soap_new_wsa5__EndpointReferenceType(soap);
        ert->Address = (char *) strdup(subscriptionEndpoint.c_str());

        // Add Topic, Message and SubscriptionReference to NotificationMessage
        wsnt__NotificationMessageHolderType *nmht = soap_new_wsnt__NotificationMessageHolderType(soap);
        nmht->Topic = tet;
        nmht->Message = *nmhtm;
        nmht->SubscriptionReference = ert;

        // Add NotificationMessage to response
        tev__PullMessagesResponse.wsnt__NotificationMessage.push_back(nmht);
    }

    message_limit--;

    if ((message_limit >= 1) && (pull_last_reboot)) {

        // LastReboot
        // Create NotificationMessage starting from bottom
        soap_dom_element *sis = new soap_dom_element(soap, NULL, "tt:SimpleItem");
        sis->add(new soap_dom_attribute(soap, NULL, "Name", "Token"));
        sis->add(new soap_dom_attribute(soap, NULL, "Value", "LR0"));
        soap_dom_element *s = new soap_dom_element(soap, NULL, "tt:Source");
        s->add(*sis);

        soap_dom_element *sid = new soap_dom_element(soap, NULL, "tt:SimpleItem");
        sid->add(new soap_dom_attribute(soap, NULL, "Name", "Status"));
        sid->add(new soap_dom_attribute(soap, NULL, "Value", iso_lb));
        soap_dom_element *d = new soap_dom_element(soap, NULL, "tt:Data");
        d->add(*sid);

        // tt:message
        soap_dom_element *m = new soap_dom_element(soap, NULL, "tt:Message");
        m->add(new soap_dom_attribute(soap, NULL, "UtcTime", iso_ct));
        m->add(*s);
        m->add(*d);

        // wsnt:Message
        _wsnt__NotificationMessageHolderType_Message *nmhtm = soap_new__wsnt__NotificationMessageHolderType_Message(soap);
        nmhtm->__any = *m;

        // wsnt:Topic
        wsnt__TopicExpressionType *tet = soap_new_wsnt__TopicExpressionType(soap);
        tet->Dialect = "http://www.onvif.org/ver10/tev/topicExpression/ConcreteSet";
        tet->__any = "tns1:Monitoring/OperatingTime/LastReboot";

        // wsnt:SubscriptionReference
        wsa5__EndpointReferenceType *ert = soap_new_wsa5__EndpointReferenceType(soap);
        ert->Address = (char *) strdup(subscriptionEndpoint.c_str());

        // Add Topic, Message and SubscriptionReference to NotificationMessage
        wsnt__NotificationMessageHolderType *nmht = soap_new_wsnt__NotificationMessageHolderType(soap);
        nmht->Topic = tet;
        nmht->Message = *nmhtm;
        nmht->SubscriptionReference = ert;

        // Add NotificationMessage to response
        tev__PullMessagesResponse.wsnt__NotificationMessage.push_back(nmht);
    }


    // Add NotificationMessage to response
    //tev__PullMessagesResponse.wsnt__NotificationMessage.push_back(nmht);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::Seek(_tev__Seek *tev__Seek, _tev__SeekResponse &tev__SeekResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::SetSynchronizationPoint(_tev__SetSynchronizationPoint *tev__SetSynchronizationPoint, _tev__SetSynchronizationPointResponse &tev__SetSynchronizationPointResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    // Fix response message
    soap->header->wsa5__Action = (char *) "http://www.onvif.org/ver10/events/wsdl/PullPointSubscription/SetSynchronizationPointResponse";

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::GetServiceCapabilities(_tev__GetServiceCapabilities *tev__GetServiceCapabilities, _tev__GetServiceCapabilitiesResponse &tev__GetServiceCapabilitiesResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    tev__GetServiceCapabilitiesResponse.Capabilities = ctx->getEventServiceCapabilities(this->soap);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::CreatePullPointSubscription(_tev__CreatePullPointSubscription *tev__CreatePullPointSubscription, _tev__CreatePullPointSubscriptionResponse &tev__CreatePullPointSubscriptionResponse)
{
    unsigned int endpointIndex, endpointLowerIndex = 1;
    std::vector<Subscription>::iterator it;
    std::vector<Subscription>::iterator endpointLowerPos;

    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;

    // Fix response message
    soap->header->wsa5__Action = (char *) "http://www.onvif.org/ver10/events/wsdl/EventPortType/CreatePullPointSubscriptionResponse";

    // Remove terminated subscriptions
    for(it = std::begin(*(ctx->get_subscriptions())); it != std::end(*(ctx->get_subscriptions()));) {
        time_t ct = time(NULL);
        if (it->get_termination_time() < ct) {
            // Don't increment if record is erased
            ctx->get_subscriptions()->erase(it);
        } else {
            ++it;
        }
    }

    // Check insert index
    for(it = std::begin(*(ctx->get_subscriptions())); it != std::end(*(ctx->get_subscriptions())); ++it) {
        endpointIndex = it->get_endpoint_index();
        if (endpointIndex == 0) {
            return SOAP_OK;
        }

        if (endpointLowerIndex == endpointIndex) {
            endpointLowerIndex++;
            endpointLowerPos = it;
        }
    }
    std::ostringstream s;
    s << endpointLowerIndex;
    std::string sEndpointLowerIndex(s.str());

    // Prepare record to insert
    LONG64 itt;
    soap_s2xsd__duration(soap, tev__CreatePullPointSubscription->InitialTerminationTime->c_str(), &itt);
    int itti = (int) (itt / 1000);

    Subscription *sub = new Subscription();
    sub->set_address("http://" + ctx->getServerIpFromClientIp(htonl(soap->ip)) + "/Subscription?Idx=" + sEndpointLowerIndex);
    time_t ct = time(NULL);
    sub->set_initial_termination_time(itti);
    sub->set_termination_time(ct + itti);

    // Save record
    if (endpointLowerIndex == ctx->get_subscriptions()->size() + 1) {
        ctx->get_subscriptions()->push_back(*sub);
    } else {
        ctx->get_subscriptions()->insert(endpointLowerPos, *sub);
    }

    // Prepare response
    wsa5__EndpointReferenceType *ert = soap_new_wsa5__EndpointReferenceType(soap);
    ert->Address = (char *) strdup(sub->get_address().c_str());

    tev__CreatePullPointSubscriptionResponse.SubscriptionReference = *ert;
    tev__CreatePullPointSubscriptionResponse.wsnt__CurrentTime = ct;
    tev__CreatePullPointSubscriptionResponse.wsnt__TerminationTime = sub->get_termination_time();

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::GetEventProperties(_tev__GetEventProperties *tev__GetEventProperties, _tev__GetEventPropertiesResponse &tev__GetEventPropertiesResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    // Fix response message
    soap->header->wsa5__Action = (char *) "http://www.onvif.org/ver10/events/wsdl/EventPortType/GetEventPropertiesResponse";

    // Setting properties
    std::string tnl("http://www.onvif.org/onvif/ver10/topics/topicns.xml");
    tev__GetEventPropertiesResponse.TopicNamespaceLocation.push_back(tnl);

    tev__GetEventPropertiesResponse.wsnt__FixedTopicSet = true;

    std::string ted("http://www.onvif.org/ver10/tev/topicExpression/ConcreteSet");
    tev__GetEventPropertiesResponse.wsnt__TopicExpressionDialect.push_back(ted);

    std::string ted2("http://docs.oasis-open.org/wsn/t-1/TopicExpression/Concrete");
    tev__GetEventPropertiesResponse.wsnt__TopicExpressionDialect.push_back(ted2);

    std::string mcfd("http://www.onvif.org/ver10/tev/messageContentFilter/ItemFilter");
    tev__GetEventPropertiesResponse.MessageContentFilterDialect.push_back(mcfd);

    std::string mcsl("http://www.onvif.org/ver10/schema/onvif.xsd");
    tev__GetEventPropertiesResponse.MessageContentSchemaLocation.push_back(mcsl);

    wstop__TopicSetType *tst = soap_new_wstop__TopicSetType(soap);
    tev__GetEventPropertiesResponse.wstop__TopicSet = tst;

    // MotionAlarm
    soap_dom_element *sids = new soap_dom_element(soap, NULL, "tt:SimpleItemDescription");
    sids->add(new soap_dom_attribute(soap, NULL, "Name", "VideoSourceToken"));
    sids->add(new soap_dom_attribute(soap, NULL, "Type", "tt:ReferenceToken"));
    soap_dom_element *s = new soap_dom_element(soap, NULL, "tt:Source");
    s->add(*sids);

    soap_dom_element *sidd = new soap_dom_element(soap, NULL, "tt:SimpleItemDescription");
    sidd->add(new soap_dom_attribute(soap, NULL, "Name", "State"));
    sidd->add(new soap_dom_attribute(soap, NULL, "Type", "xsd:boolean"));
    soap_dom_element *d = new soap_dom_element(soap, NULL, "tt:Data");
    d->add(*sidd);

    soap_dom_element *md = new soap_dom_element(soap, NULL, "tt:MessageDescription");
    md->add(new soap_dom_attribute(soap, NULL, "IsProperty", "true"));
    md->add(*s);
    md->add(*d);

    soap_dom_element *ma = new soap_dom_element(soap, NULL, "MotionAlarm");
    ma->add(new soap_dom_attribute(soap, NULL, "wstop:topic", "true"));
    ma->add(*md);

    soap_dom_element *vs = new soap_dom_element(soap, NULL, "tns1:VideoSource");
    vs->add(*ma);

    // ProcessorUsage
    soap_dom_element *sids2 = new soap_dom_element(soap, NULL, "tt:SimpleItemDescription");
    sids2->add(new soap_dom_attribute(soap, NULL, "Name", "Token"));
    sids2->add(new soap_dom_attribute(soap, NULL, "Type", "tt:ReferenceToken"));
    soap_dom_element *s2 = new soap_dom_element(soap, NULL, "tt:Source");
    s2->add(*sids2);

    soap_dom_element *sidd2 = new soap_dom_element(soap, NULL, "tt:SimpleItemDescription");
    sidd2->add(new soap_dom_attribute(soap, NULL, "Name", "Value"));
    sidd2->add(new soap_dom_attribute(soap, NULL, "Type", "xsd:float"));
    soap_dom_element *d2 = new soap_dom_element(soap, NULL, "tt:Data");
    d2->add(*sidd2);

    soap_dom_element *md2 = new soap_dom_element(soap, NULL, "tt:MessageDescription");
    md2->add(new soap_dom_attribute(soap, NULL, "IsProperty", "true"));
    md2->add(*s2);
    md2->add(*d2);

    soap_dom_element *pu = new soap_dom_element(soap, NULL, "ProcessorUsage");
    pu->add(new soap_dom_attribute(soap, NULL, "wstop:topic", "true"));
    pu->add(*md2);

    soap_dom_element *mon1 = new soap_dom_element(soap, NULL, "tns1:Monitoring");
    mon1->add(*pu);

    // LastReboot
    soap_dom_element *sids3 = new soap_dom_element(soap, NULL, "tt:SimpleItemDescription");
    sids3->add(new soap_dom_attribute(soap, NULL, "Name", "Token"));
    sids3->add(new soap_dom_attribute(soap, NULL, "Type", "tt:ReferenceToken"));
    soap_dom_element *s3 = new soap_dom_element(soap, NULL, "tt:Source");
    s3->add(*sids3);

    soap_dom_element *sidd3 = new soap_dom_element(soap, NULL, "tt:SimpleItemDescription");
    sidd3->add(new soap_dom_attribute(soap, NULL, "Name", "Status"));
    sidd3->add(new soap_dom_attribute(soap, NULL, "Type", "xsd:dateTime"));
    soap_dom_element *d3 = new soap_dom_element(soap, NULL, "tt:Data");
    d3->add(*sidd3);

    soap_dom_element *md3 = new soap_dom_element(soap, NULL, "tt:MessageDescription");
    md3->add(new soap_dom_attribute(soap, NULL, "IsProperty", "true"));
    md3->add(*s3);
    md3->add(*d3);

    soap_dom_element *lr = new soap_dom_element(soap, NULL, "LastReboot");
    lr->add(new soap_dom_attribute(soap, NULL, "wstop:topic", "true"));
    lr->add(*md3);

    soap_dom_element *ot = new soap_dom_element(soap, NULL, "OperatingTime");
    ot->add(new soap_dom_attribute(soap, NULL, "wstop:topic", "true"));
    ot->add(*lr);

    soap_dom_element *mon2 = new soap_dom_element(soap, NULL, "tns1:Monitoring");
    mon2->add(*ot);

    tev__GetEventPropertiesResponse.wstop__TopicSet->__any.push_back(*vs);
    tev__GetEventPropertiesResponse.wstop__TopicSet->__any.push_back(*mon1);
    tev__GetEventPropertiesResponse.wstop__TopicSet->__any.push_back(*mon2);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::Unsubscribe(_wsnt__Unsubscribe *wsnt__Unsubscribe, _wsnt__UnsubscribeResponse &wsnt__UnsubscribeResponse)
{
    std::vector<Subscription>::iterator it;

    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;

    // Fix response message
    soap->header->wsa5__Action = (char *) "http://docs.oasis-open.org/wsn/bw-2/SubscriptionManager/UnsubscribeResponse";

    // Get subscription endpoint from request header
    std::string subscriptionEndpoint = soap->header->wsa5__To;

    for(it = std::begin(*(ctx->get_subscriptions())); it != std::end(*(ctx->get_subscriptions())); ++it) {
        if (it->get_address().compare(subscriptionEndpoint) == 0) {
            ctx->get_subscriptions()->erase(it);
            break;
        }
    }

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::AddEventBroker(_tev__AddEventBroker *tev__AddEventBroker, _tev__AddEventBrokerResponse &tev__AddEventBrokerResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::DeleteEventBroker(_tev__DeleteEventBroker *tev__DeleteEventBroker, _tev__DeleteEventBrokerResponse &tev__DeleteEventBrokerResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::GetEventBrokers(_tev__GetEventBrokers *tev__GetEventBrokers, _tev__GetEventBrokersResponse &tev__GetEventBrokersResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::Renew(_wsnt__Renew *wsnt__Renew, _wsnt__RenewResponse &wsnt__RenewResponse)
{
    std::vector<Subscription>::iterator it;

    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;

    // Fix response message
    soap->header->wsa5__Action = (char *) "http://docs.oasis-open.org/wsn/bw-2/SubscriptionManager/RenewResponse";

    // Get subscription endpoint from request header
    std::string subscriptionEndpoint = soap->header->wsa5__To;

    bool sub_found = false;
    time_t ct = time(NULL);
    for(it = std::begin(*(ctx->get_subscriptions())); it != std::end(*(ctx->get_subscriptions())); ++it) {
        if (it->get_address().compare(subscriptionEndpoint) == 0) {
            it->set_termination_time(ct + it->get_initial_termination_time());
            sub_found = true;
            break;
        }
    }

    if (!sub_found) {
        return SOAP_OK;
    }

    wsnt__RenewResponse.CurrentTime = &ct;
    wsnt__RenewResponse.TerminationTime = it->get_termination_time();

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::Subscribe(_wsnt__Subscribe *wsnt__Subscribe, _wsnt__SubscribeResponse &wsnt__SubscribeResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::GetCurrentMessage(_wsnt__GetCurrentMessage *wsnt__GetCurrentMessage, _wsnt__GetCurrentMessageResponse &wsnt__GetCurrentMessageResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::Notify(_wsnt__Notify *wsnt__Notify)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::GetMessages(_wsnt__GetMessages *wsnt__GetMessages, _wsnt__GetMessagesResponse &wsnt__GetMessagesResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::DestroyPullPoint(_wsnt__DestroyPullPoint *wsnt__DestroyPullPoint, _wsnt__DestroyPullPointResponse &wsnt__DestroyPullPointResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::CreatePullPoint(_wsnt__CreatePullPoint *wsnt__CreatePullPoint, _wsnt__CreatePullPointResponse &wsnt__CreatePullPointResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::PauseSubscription(_wsnt__PauseSubscription *wsnt__PauseSubscription, _wsnt__PauseSubscriptionResponse &wsnt__PauseSubscriptionResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::ResumeSubscription(_wsnt__ResumeSubscription *wsnt__ResumeSubscription, _wsnt__ResumeSubscriptionResponse &wsnt__ResumeSubscriptionResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::Unsubscribe_(_wsnt__Unsubscribe *wsnt__Unsubscribe, _wsnt__UnsubscribeResponse &wsnt__UnsubscribeResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::Notify_(_wsnt__Notify *wsnt__Notify)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::Renew_(_wsnt__Renew *wsnt__Renew, _wsnt__RenewResponse &wsnt__RenewResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::Unsubscribe__(_wsnt__Unsubscribe *wsnt__Unsubscribe, _wsnt__UnsubscribeResponse &wsnt__UnsubscribeResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    return SOAP_OK;
}
