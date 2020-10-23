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
#include <semaphore.h>

#define IPCSYS_DB "/mnt/mtd/db/ipcsys.db"



int PullPointSubscriptionBindingService::PullMessages(_tev__PullMessages *tev__PullMessages, _tev__PullMessagesResponse &tev__PullMessagesResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);
    printf("EVENT: %s\n", __FUNCTION__);

    DetectedEvent *alarm;
    DetectedEvent *sysinfo;
    bool pull_motion_alarm = false;
    bool pull_processor_usage = false;
    bool pull_last_reboot = false;
    time_t request_t, alarm_t, sysinfo_t;
    bool alarm_value = false;
    float sysinfo_f_value = 0;
    time_t sysinfo_t_value;
    struct tm *timeinfo_utc;
    char iso_ct[32];
    char iso_lb[32];
    char load[32];
    int message_limit;
    std::string subscriptionEndpoint;
    std::vector<Subscription>::iterator it;
    char iso_utc[32] = "1970-01-01T00:00:00Z";
    struct tm *timeinfoct;
    struct tm *timeinfolb;

    time(&request_t);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;

    // Fix response message
    soap->header->wsa5__Action = (char *) "http://www.onvif.org/ver10/events/wsdl/PullPointSubscription/PullMessagesResponse";

    // Get subscription endpoint from request header
    if (soap->header->wsa5__To != NULL) {
        subscriptionEndpoint.assign(soap->header->wsa5__To);
    } else {
        subscriptionEndpoint.assign(soap->endpoint);
    }

    // Get Message Limit
    message_limit = tev__PullMessages->MessageLimit;

    // Find subscription
    bool sub_found = false;
    for(it = std::begin(*(ctx->get_subscriptions())); it != std::end(*(ctx->get_subscriptions())); ++it) {
        if (it->get_address().compare(subscriptionEndpoint) == 0) {
            if (it->get_termination_time() < request_t) {
                ctx->get_subscriptions()->erase(it);
            } else {
                sub_found = true;
            }
            break;
        }
    }

    if (sub_found) {
        it->set_termination_time(request_t + it->get_initial_termination_time());
    } else {
        return SOAP_FAULT;
    }

    printf("size: %d\n", ctx->get_subscriptions()->size());

    // MotionAlarm
    alarm = ctx->get_last_motion_alarm();
    if(!alarm->get_sent()) {
        sem_wait(alarm->get_sem());
        pull_motion_alarm = true;
        alarm_t = alarm->get_time();
        alarm_value = alarm->get_b_value();
        alarm->set_sent(true);
        sem_post(alarm->get_sem());
    }

    // ProcessorUsage and LastReboot
    sysinfo = ctx->get_sysinfo();
    if(!sysinfo->get_sent()) {
        sem_wait(sysinfo->get_sem());
        pull_processor_usage = true;
        pull_last_reboot = true;
        sysinfo_t = sysinfo->get_time();
        sysinfo_f_value = sysinfo->get_f_value();
        sysinfo_t_value = sysinfo->get_t_value();
        sysinfo->set_sent(true);
        sem_post(sysinfo->get_sem());
    }

    // Set CurrentTime and TerminationTime
    tev__PullMessagesResponse.CurrentTime = request_t;
    tev__PullMessagesResponse.TerminationTime = it->get_termination_time();

    if((!pull_motion_alarm) && (!pull_processor_usage) && (!pull_last_reboot)) {
        return SOAP_OK;
    }

    if ((message_limit >= 1) && (pull_motion_alarm)) {

        timeinfo_utc = gmtime(&alarm_t);
        sprintf(iso_utc, "%04d-%02d-%02dT%02d:%02d:%02dZ", timeinfo_utc->tm_year + 1900, timeinfo_utc->tm_mon + 1, timeinfo_utc->tm_mday, timeinfo_utc->tm_hour, timeinfo_utc->tm_min, timeinfo_utc->tm_sec);

        // MotionAlarm
        // Create NotificationMessage starting from bottom
        xsd__anyType *sis = soap_elt_new(soap, NULL, "tt:SimpleItem");
//        soap_dom_element *sis = new soap_dom_element(soap, NULL, "tt:SimpleItem");
        soap_att_text(soap_att(sis, NULL, "Name"), "VideoSourceToken");
//        sis->add(new soap_dom_attribute(soap, NULL, "Name", "VideoSourceToken"));
        soap_att_text(soap_att(sis, NULL, "Value"), "MA0");
//        sis->add(new soap_dom_attribute(soap, NULL, "Value", "MA0"));
        xsd__anyType *s = soap_elt_new(soap, NULL, "tt:Source");
//        soap_dom_element *s = new soap_dom_element(soap, NULL, "tt:Source");
        s->add(*sis);

        xsd__anyType *sid = soap_elt_new(soap, NULL, "tt:SimpleItem");
//        soap_dom_element *sid = new soap_dom_element(soap, NULL, "tt:SimpleItem");
        soap_att_text(soap_att(sid, NULL, "Name"), "State");
//        sid->add(new soap_dom_attribute(soap, NULL, "Name", "State"));
        if (alarm_value) {
            soap_att_text(soap_att(sid, NULL, "Value"), "true");
//            sid->add(new soap_dom_attribute(soap, NULL, "Value", "true"));
        } else {
            soap_att_text(soap_att(sid, NULL, "Value"), "false");
//            sid->add(new soap_dom_attribute(soap, NULL, "Value", "false"));
        }
        xsd__anyType *d = soap_elt_new(soap, NULL, "tt:Data");
//        soap_dom_element *d = new soap_dom_element(soap, NULL, "tt:Data");
        d->add(*sid);

        // tt:message
        xsd__anyType *m = soap_elt_new(soap, NULL, "tt:Message");
//        soap_dom_element *m = new soap_dom_element(soap, NULL, "tt:Message");
        soap_att_text(soap_att(m, NULL, "UtcTime"), iso_utc);
//        m->add(new soap_dom_attribute(soap, NULL, "UtcTime", iso_utc));
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

        sprintf(load, "%.2f", sysinfo_f_value);
        timeinfoct = gmtime(&sysinfo_t);
        sprintf(iso_ct, "%04d-%02d-%02dT%02d:%02d:%02dZ", timeinfoct->tm_year + 1900, timeinfoct->tm_mon + 1, timeinfoct->tm_mday, timeinfoct->tm_hour, timeinfoct->tm_min, timeinfoct->tm_sec);

        // ProcessorUsage
        // Create NotificationMessage starting from bottom
        xsd__anyType *sis = soap_elt_new(soap, NULL, "tt:SimpleItem");
//        soap_dom_element *sis = new soap_dom_element(soap, NULL, "tt:SimpleItem");
        soap_att_text(soap_att(sis, NULL, "Name"), "Token");
//        sis->add(new soap_dom_attribute(soap, NULL, "Name", "Token"));
        soap_att_text(soap_att(sis, NULL, "Value"), "PU0");
//        sis->add(new soap_dom_attribute(soap, NULL, "Value", "PU0"));
        xsd__anyType *s = soap_elt_new(soap, NULL, "tt:Source");
//        soap_dom_element *s = new soap_dom_element(soap, NULL, "tt:Source");
        s->add(*sis);

        xsd__anyType *sid = soap_elt_new(soap, NULL, "tt:SimpleItem");
//        soap_dom_element *sid = new soap_dom_element(soap, NULL, "tt:SimpleItem");
        soap_att_text(soap_att(sid, NULL, "Name"), "Value");
//        sid->add(new soap_dom_attribute(soap, NULL, "Name", "Value"));
        soap_att_text(soap_att(sid, NULL, "Value"), load);
//        sid->add(new soap_dom_attribute(soap, NULL, "Value", load));
        xsd__anyType *d = soap_elt_new(soap, NULL, "tt:Data");
//        soap_dom_element *d = new soap_dom_element(soap, NULL, "tt:Data");
        d->add(*sid);

        // tt:message
        xsd__anyType *m = soap_elt_new(soap, NULL, "tt:Message");
//        soap_dom_element *m = new soap_dom_element(soap, NULL, "tt:Message");
        soap_att_text(soap_att(m, NULL, "UtcTime"), iso_ct);
//        m->add(new soap_dom_attribute(soap, NULL, "UtcTime", iso_ct));
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

        timeinfolb = gmtime(&sysinfo_t_value);
        sprintf(iso_lb, "%04d-%02d-%02dT%02d:%02d:%02dZ", timeinfolb->tm_year + 1900, timeinfolb->tm_mon + 1, timeinfolb->tm_mday, timeinfolb->tm_hour, timeinfolb->tm_min, timeinfolb->tm_sec);
        timeinfoct = gmtime(&sysinfo_t);
        sprintf(iso_ct, "%04d-%02d-%02dT%02d:%02d:%02dZ", timeinfoct->tm_year + 1900, timeinfoct->tm_mon + 1, timeinfoct->tm_mday, timeinfoct->tm_hour, timeinfoct->tm_min, timeinfoct->tm_sec);

        // LastReboot
        // Create NotificationMessage starting from bottom
        xsd__anyType *sis = soap_elt_new(soap, NULL, "tt:SimpleItem");
//        soap_dom_element *sis = new soap_dom_element(soap, NULL, "tt:SimpleItem");
        soap_att_text(soap_att(sis, NULL, "Name"), "Token");
//        sis->add(new soap_dom_attribute(soap, NULL, "Name", "Token"));
        soap_att_text(soap_att(sis, NULL, "Value"), "LR0");
//        sis->add(new soap_dom_attribute(soap, NULL, "Value", "LR0"));
        xsd__anyType *s = soap_elt_new(soap, NULL, "tt:Source");
//        soap_dom_element *s = new soap_dom_element(soap, NULL, "tt:Source");
        s->add(*sis);

        xsd__anyType *sid = soap_elt_new(soap, NULL, "tt:SimpleItem");
//        soap_dom_element *sid = new soap_dom_element(soap, NULL, "tt:SimpleItem");
        soap_att_text(soap_att(sid, NULL, "Name"), "Status");
//        sid->add(new soap_dom_attribute(soap, NULL, "Name", "Status"));
        soap_att_text(soap_att(sid, NULL, "Value"), iso_lb);
//        sid->add(new soap_dom_attribute(soap, NULL, "Value", iso_lb));
        xsd__anyType *d = soap_elt_new(soap, NULL, "tt:Data");
//        soap_dom_element *d = new soap_dom_element(soap, NULL, "tt:Data");
        d->add(*sid);

        // tt:message
        xsd__anyType *m = soap_elt_new(soap, NULL, "tt:Message");
//        soap_dom_element *m = new soap_dom_element(soap, NULL, "tt:Message");
        soap_att_text(soap_att(m, NULL, "UtcTime"), iso_ct);
//        m->add(new soap_dom_attribute(soap, NULL, "UtcTime", iso_ct));
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

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::Seek(_tev__Seek *tev__Seek, _tev__SeekResponse &tev__SeekResponse)
{
    SOAP_EMPTY_HANDLER(tev__Seek, "PullPointSubscription");
}

int PullPointSubscriptionBindingService::SetSynchronizationPoint(_tev__SetSynchronizationPoint *tev__SetSynchronizationPoint, _tev__SetSynchronizationPointResponse &tev__SetSynchronizationPointResponse)
{
    UNUSED(tev__SetSynchronizationPoint);
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    // Fix response message
    soap->header->wsa5__Action = (char *) "http://www.onvif.org/ver10/events/wsdl/PullPointSubscription/SetSynchronizationPointResponse";

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::GetServiceCapabilities(_tev__GetServiceCapabilities *tev__GetServiceCapabilities, _tev__GetServiceCapabilitiesResponse &tev__GetServiceCapabilitiesResponse)
{
    UNUSED(tev__GetServiceCapabilities);
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    ServiceContext* ctx = (ServiceContext*)this->soap->user;
    tev__GetServiceCapabilitiesResponse.Capabilities = ctx->getEventServiceCapabilities(this->soap);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::CreatePullPointSubscription(_tev__CreatePullPointSubscription *tev__CreatePullPointSubscription, _tev__CreatePullPointSubscriptionResponse &tev__CreatePullPointSubscriptionResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);
    printf("EVENT: %s\n", __FUNCTION__);

    unsigned int endpointIndex, endpointLowerIndex = 1;
    std::vector<Subscription>::iterator it;
    std::vector<Subscription>::iterator endpointLowerPos;

    ServiceContext* ctx = (ServiceContext*)this->soap->user;

    // Fix response message
    soap->header->wsa5__Action = (char *) "http://www.onvif.org/ver10/events/wsdl/EventPortType/CreatePullPointSubscriptionResponse";

    printf("Remove terminated subscriptions\n");
    // Remove terminated subscriptions
    time_t ct = time(NULL);
    for(it = std::begin(*(ctx->get_subscriptions())); it != std::end(*(ctx->get_subscriptions()));) {
        if (it->get_termination_time() < ct) {
            // Don't increment if record is erased
            ctx->get_subscriptions()->erase(it);
        } else {
            ++it;
        }
    }

    printf("Check insert index\n");
    // Check insert index
    for(it = std::begin(*(ctx->get_subscriptions())); it != std::end(*(ctx->get_subscriptions())); ++it) {
        endpointIndex = it->get_endpoint_index();
        if (endpointIndex == 0) {
            return SOAP_OK;
        }

        if (endpointLowerIndex == endpointIndex) {
            endpointLowerIndex++;
            endpointLowerPos = it;
            break;
        }
    }
    std::ostringstream s;
    s << endpointLowerIndex;
    std::string sEndpointLowerIndex(s.str());

    printf("Prepare record to insert\n");
    // Prepare record to insert
    LONG64 itt;
    if (tev__CreatePullPointSubscription->InitialTerminationTime == NULL) {
        soap_s2xsd__duration(soap, "PT60S", &itt);
    } else {
        if (tev__CreatePullPointSubscription->InitialTerminationTime->substr(0, 1).compare("P") == 0) {
            soap_s2xsd__duration(soap, tev__CreatePullPointSubscription->InitialTerminationTime->c_str(), &itt);
        } else {
            time_t t;
            soap_s2dateTime(soap, tev__CreatePullPointSubscription->InitialTerminationTime->c_str(), &t);
            itt =  (t - ct) * 1000;
        }
    }
    int itti = (int) (itt / 1000);

    Subscription *sub = new Subscription();
    sub->set_address("http://" + ctx->getServerIpFromClientIp(htonl(soap->ip)) + "/Subscription?Idx=" + sEndpointLowerIndex);
    sub->set_initial_termination_time(itti);
    sub->set_termination_time(ct + itti);

    printf("Save record\n");
    // Save record
    if (endpointLowerIndex == ctx->get_subscriptions()->size() + 1) {
        ctx->get_subscriptions()->push_back(*sub);
    } else {
        ctx->get_subscriptions()->insert(endpointLowerPos, *sub);
    }

    printf("Prepare response\n");
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
    UNUSED(tev__GetEventProperties);
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
    xsd__anyType *sids = soap_elt_new(soap, NULL, "tt:SimpleItemDescription");
//    soap_dom_element *sids = new soap_dom_element(soap, NULL, "tt:SimpleItemDescription");
    soap_att_text(soap_att(sids, NULL, "Name"), "VideoSourceToken");
//    sids->add(new soap_dom_attribute(soap, NULL, "Name", "VideoSourceToken"));
    soap_att_text(soap_att(sids, NULL, "Type"), "tt:ReferenceToken");
//    sids->add(new soap_dom_attribute(soap, NULL, "Type", "tt:ReferenceToken"));
    xsd__anyType *s = soap_elt_new(soap, NULL, "tt:Source");
//    soap_dom_element *s = new soap_dom_element(soap, NULL, "tt:Source");
    s->add(*sids);

    xsd__anyType *sidd = soap_elt_new(soap, NULL, "tt:SimpleItemDescription");
//    soap_dom_element *sidd = new soap_dom_element(soap, NULL, "tt:SimpleItemDescription");
    soap_att_text(soap_att(sidd, NULL, "Name"), "State");
//    sidd->add(new soap_dom_attribute(soap, NULL, "Name", "State"));
    soap_att_text(soap_att(sidd, NULL, "Type"), "xsd:boolean");
//    sidd->add(new soap_dom_attribute(soap, NULL, "Type", "xsd:boolean"));
    xsd__anyType *d = soap_elt_new(soap, NULL, "tt:Data");
//    soap_dom_element *d = new soap_dom_element(soap, NULL, "tt:Data");
    d->add(*sidd);

    xsd__anyType *md = soap_elt_new(soap, NULL, "tt:MessageDescription");
//    soap_dom_element *md = new soap_dom_element(soap, NULL, "tt:MessageDescription");
    soap_att_text(soap_att(md, NULL, "IsProperty"), "true");
//    md->add(new soap_dom_attribute(soap, NULL, "IsProperty", "true"));
    md->add(*s);
    md->add(*d);

    xsd__anyType *ma = soap_elt_new(soap, NULL, "MotionAlarm");
//    soap_dom_element *ma = new soap_dom_element(soap, NULL, "MotionAlarm");
    soap_att_text(soap_att(ma, NULL, "wstop:topic"), "true");
//    ma->add(new soap_dom_attribute(soap, NULL, "wstop:topic", "true"));
    ma->add(*md);

    xsd__anyType *vs = soap_elt_new(soap, NULL, "tns1:VideoSource");
//    soap_dom_element *vs = new soap_dom_element(soap, NULL, "tns1:VideoSource");
    vs->add(*ma);

    // ProcessorUsage
    xsd__anyType *sids2 = soap_elt_new(soap, NULL, "tt:SimpleItemDescription");
//    soap_dom_element *sids2 = new soap_dom_element(soap, NULL, "tt:SimpleItemDescription");
    soap_att_text(soap_att(sids2, NULL, "Name"), "Token");
//    sids2->add(new soap_dom_attribute(soap, NULL, "Name", "Token"));
    soap_att_text(soap_att(sids2, NULL, "Type"), "tt:ReferenceToken");
//    sids2->add(new soap_dom_attribute(soap, NULL, "Type", "tt:ReferenceToken"));
    xsd__anyType *s2 = soap_elt_new(soap, NULL, "tt:Source");
//    soap_dom_element *s2 = new soap_dom_element(soap, NULL, "tt:Source");
    s2->add(*sids2);

    xsd__anyType *sidd2 = soap_elt_new(soap, NULL, "tt:SimpleItemDescription");
//    soap_dom_element *sidd2 = new soap_dom_element(soap, NULL, "tt:SimpleItemDescription");
    soap_att_text(soap_att(sidd2, NULL, "Name"), "Value");
//    sidd2->add(new soap_dom_attribute(soap, NULL, "Name", "Value"));
    soap_att_text(soap_att(sidd2, NULL, "Type"), "xsd:float");
//    sidd2->add(new soap_dom_attribute(soap, NULL, "Type", "xsd:float"));
    xsd__anyType *d2 = soap_elt_new(soap, NULL, "tt:Data");
//    soap_dom_element *d2 = new soap_dom_element(soap, NULL, "tt:Data");
    d2->add(*sidd2);

    xsd__anyType *md2 = soap_elt_new(soap, NULL, "tt:MessageDescription");
//    soap_dom_element *md2 = new soap_dom_element(soap, NULL, "tt:MessageDescription");
    soap_att_text(soap_att(md2, NULL, "IsProperty"), "true");
//    md2->add(new soap_dom_attribute(soap, NULL, "IsProperty", "true"));
    md2->add(*s2);
    md2->add(*d2);

    xsd__anyType *pu = soap_elt_new(soap, NULL, "ProcessorUsage");
//    soap_dom_element *pu = new soap_dom_element(soap, NULL, "ProcessorUsage");
    soap_att_text(soap_att(pu, NULL, "wstop:topic"), "true");
//    pu->add(new soap_dom_attribute(soap, NULL, "wstop:topic", "true"));
    pu->add(*md2);

    xsd__anyType *mon1 = soap_elt_new(soap, NULL, "tns1:Monitoring");
//    soap_dom_element *mon1 = new soap_dom_element(soap, NULL, "tns1:Monitoring");
    mon1->add(*pu);

    // LastReboot
    xsd__anyType *sids3 = soap_elt_new(soap, NULL, "tt:SimpleItemDescription");
//    soap_dom_element *sids3 = new soap_dom_element(soap, NULL, "tt:SimpleItemDescription");
    soap_att_text(soap_att(sids3, NULL, "Name"), "Token");
//    sids3->add(new soap_dom_attribute(soap, NULL, "Name", "Token"));
    soap_att_text(soap_att(sids3, NULL, "Type"), "tt:ReferenceToken");
//    sids3->add(new soap_dom_attribute(soap, NULL, "Type", "tt:ReferenceToken"));
    xsd__anyType *s3 = soap_elt_new(soap, NULL, "tt:Source");
//    soap_dom_element *s3 = new soap_dom_element(soap, NULL, "tt:Source");
    s3->add(*sids3);

    xsd__anyType *sidd3 = soap_elt_new(soap, NULL, "tt:SimpleItemDescription");
//    soap_dom_element *sidd3 = new soap_dom_element(soap, NULL, "tt:SimpleItemDescription");
    soap_att_text(soap_att(sidd3, NULL, "Name"), "Status");
//    sidd3->add(new soap_dom_attribute(soap, NULL, "Name", "Status"));
    soap_att_text(soap_att(sidd3, NULL, "Type"), "xsd:dateTime");
//    sidd3->add(new soap_dom_attribute(soap, NULL, "Type", "xsd:dateTime"));
    xsd__anyType *d3 = soap_elt_new(soap, NULL, "tt:Data");
//    soap_dom_element *d3 = new soap_dom_element(soap, NULL, "tt:Data");
    d3->add(*sidd3);

    xsd__anyType *md3 = soap_elt_new(soap, NULL, "tt:MessageDescription");
//    soap_dom_element *md3 = new soap_dom_element(soap, NULL, "tt:MessageDescription");
    soap_att_text(soap_att(md3, NULL, "IsProperty"), "true");
//    md3->add(new soap_dom_attribute(soap, NULL, "IsProperty", "true"));
    md3->add(*s3);
    md3->add(*d3);

    xsd__anyType *lr = soap_elt_new(soap, NULL, "LastReboot");
//    soap_dom_element *lr = new soap_dom_element(soap, NULL, "LastReboot");
    soap_att_text(soap_att(lr, NULL, "wstop:topic"), "true");
//    lr->add(new soap_dom_attribute(soap, NULL, "wstop:topic", "true"));
    lr->add(*md3);

    xsd__anyType *ot = soap_elt_new(soap, NULL, "OperatingTime");
//    soap_dom_element *ot = new soap_dom_element(soap, NULL, "OperatingTime");
    soap_att_text(soap_att(ot, NULL, "wstop:topic"), "true");
//    ot->add(new soap_dom_attribute(soap, NULL, "wstop:topic", "true"));
    ot->add(*lr);

    xsd__anyType *mon2 = soap_elt_new(soap, NULL, "tns1:Monitoring");
//    soap_dom_element *mon2 = new soap_dom_element(soap, NULL, "tns1:Monitoring");
    mon2->add(*ot);

    tev__GetEventPropertiesResponse.wstop__TopicSet->__any.push_back(*vs);
    tev__GetEventPropertiesResponse.wstop__TopicSet->__any.push_back(*mon1);
    tev__GetEventPropertiesResponse.wstop__TopicSet->__any.push_back(*mon2);

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::Unsubscribe(_wsnt__Unsubscribe *wsnt__Unsubscribe, _wsnt__UnsubscribeResponse &wsnt__UnsubscribeResponse)
{
    UNUSED(wsnt__Unsubscribe);
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);

    std::string subscriptionEndpoint;
    std::vector<Subscription>::iterator it;

    ServiceContext* ctx = (ServiceContext*)this->soap->user;

    // Fix response message
    soap->header->wsa5__Action = (char *) "http://docs.oasis-open.org/wsn/bw-2/SubscriptionManager/UnsubscribeResponse";

    // Get subscription endpoint from request header
    if (soap->header->wsa5__To != NULL) {
        subscriptionEndpoint.assign(soap->header->wsa5__To);
    } else {
        subscriptionEndpoint.assign(soap->endpoint);
    }

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
    SOAP_EMPTY_HANDLER(tev__AddEventBroker, "PullPointSubscription");
}

int PullPointSubscriptionBindingService::DeleteEventBroker(_tev__DeleteEventBroker *tev__DeleteEventBroker, _tev__DeleteEventBrokerResponse &tev__DeleteEventBrokerResponse)
{
    SOAP_EMPTY_HANDLER(tev__DeleteEventBroker, "PullPointSubscription");
}

int PullPointSubscriptionBindingService::GetEventBrokers(_tev__GetEventBrokers *tev__GetEventBrokers, _tev__GetEventBrokersResponse &tev__GetEventBrokersResponse)
{
    SOAP_EMPTY_HANDLER(tev__GetEventBrokers, "PullPointSubscription");
}

int PullPointSubscriptionBindingService::Renew(_wsnt__Renew *wsnt__Renew, _wsnt__RenewResponse &wsnt__RenewResponse)
{
    DEBUG_MSG("EVENT: %s\n", __FUNCTION__);
    printf("EVENT: %s\n", __FUNCTION__);

    std::string subscriptionEndpoint;
    std::vector<Subscription>::iterator it;

    ServiceContext* ctx = (ServiceContext*)this->soap->user;

    // Get subscription endpoint from request header
    if (soap->header->wsa5__To != NULL) {
        subscriptionEndpoint.assign(soap->header->wsa5__To);
    } else {
        subscriptionEndpoint.assign(soap->endpoint);
    }

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
        // Fix response message
        soap->header->wsa5__Action = (char *) "http://www.w3.org/2005/08/addressing/soap/fault";
//        struct SOAP_ENV__Code *subcode1 = soap_new_SOAP_ENV__Code(soap);
//        subcode1->SOAP_ENV__Value = (char*)"wsrf-rw:ResourceUnknownFault";
//        subcode1->SOAP_ENV__Subcode = NULL;
//        soap->fault->SOAP_ENV__Code->SOAP_ENV__Subcode = subcode1;
        soap_receiver_fault(soap, "wsrf-rw:ResourceUnknownFault", NULL);
        return SOAP_FAULT;
    }

    // Fix response message
    soap->header->wsa5__Action = (char *) "http://docs.oasis-open.org/wsn/bw-2/SubscriptionManager/RenewResponse";

    LONG64 tt;
    if (wsnt__Renew->TerminationTime == NULL) {
        tt = it->get_initial_termination_time() * 1000;
    } else {
        if (wsnt__Renew->TerminationTime->substr(0, 1).compare("P") == 0) {
            soap_s2xsd__duration(soap, wsnt__Renew->TerminationTime->c_str(), &tt);
        } else {
            time_t t;
            soap_s2dateTime(soap, wsnt__Renew->TerminationTime->c_str(), &t);
            if (t <= ct) {
                return SOAP_FAULT;
            } else {
                tt = (t - ct) * 1000;
            }
        }
    }
    int tti = (int) (tt / 1000);

    it->set_termination_time(ct + tti);

    wsnt__RenewResponse.CurrentTime = soap_new_dateTime(soap, 1);
    *wsnt__RenewResponse.CurrentTime = ct;
    wsnt__RenewResponse.TerminationTime = it->get_termination_time();

    return SOAP_OK;
}

int PullPointSubscriptionBindingService::Subscribe(_wsnt__Subscribe *wsnt__Subscribe, _wsnt__SubscribeResponse &wsnt__SubscribeResponse)
{
    SOAP_EMPTY_HANDLER(wsnt__Subscribe, "PullPointSubscription");
}

int PullPointSubscriptionBindingService::GetCurrentMessage(_wsnt__GetCurrentMessage *wsnt__GetCurrentMessage, _wsnt__GetCurrentMessageResponse &wsnt__GetCurrentMessageResponse)
{
    SOAP_EMPTY_HANDLER(wsnt__GetCurrentMessage, "PullPointSubscription");
}

int PullPointSubscriptionBindingService::Notify(_wsnt__Notify *wsnt__Notify)
{
    DEBUG_MSG("PullPointSubscription: %s\n", __FUNCTION__);
    return SOAP_OK;
}

int PullPointSubscriptionBindingService::GetMessages(_wsnt__GetMessages *wsnt__GetMessages, _wsnt__GetMessagesResponse &wsnt__GetMessagesResponse)
{
    SOAP_EMPTY_HANDLER(wsnt__GetMessages, "PullPointSubscription");
}

int PullPointSubscriptionBindingService::DestroyPullPoint(_wsnt__DestroyPullPoint *wsnt__DestroyPullPoint, _wsnt__DestroyPullPointResponse &wsnt__DestroyPullPointResponse)
{
    SOAP_EMPTY_HANDLER(wsnt__DestroyPullPoint, "PullPointSubscription");
}

int PullPointSubscriptionBindingService::CreatePullPoint(_wsnt__CreatePullPoint *wsnt__CreatePullPoint, _wsnt__CreatePullPointResponse &wsnt__CreatePullPointResponse)
{
    SOAP_EMPTY_HANDLER(wsnt__CreatePullPoint, "PullPointSubscription");
}

int PullPointSubscriptionBindingService::PauseSubscription(_wsnt__PauseSubscription *wsnt__PauseSubscription, _wsnt__PauseSubscriptionResponse &wsnt__PauseSubscriptionResponse)
{
    SOAP_EMPTY_HANDLER(wsnt__PauseSubscription, "PullPointSubscription");
}

int PullPointSubscriptionBindingService::ResumeSubscription(_wsnt__ResumeSubscription *wsnt__ResumeSubscription, _wsnt__ResumeSubscriptionResponse &wsnt__ResumeSubscriptionResponse)
{
    SOAP_EMPTY_HANDLER(wsnt__ResumeSubscription, "PullPointSubscription");
}

int PullPointSubscriptionBindingService::Unsubscribe_(_wsnt__Unsubscribe *wsnt__Unsubscribe, _wsnt__UnsubscribeResponse &wsnt__UnsubscribeResponse)
{
    SOAP_EMPTY_HANDLER(wsnt__Unsubscribe, "PullPointSubscription");
}

int PullPointSubscriptionBindingService::Notify_(_wsnt__Notify *wsnt__Notify)
{
    DEBUG_MSG("PullPointSubscription: %s\n", __FUNCTION__);
    return SOAP_OK;
}

int PullPointSubscriptionBindingService::Renew_(_wsnt__Renew *wsnt__Renew, _wsnt__RenewResponse &wsnt__RenewResponse)
{
    SOAP_EMPTY_HANDLER(wsnt__Renew, "PullPointSubscription");
}

int PullPointSubscriptionBindingService::Unsubscribe__(_wsnt__Unsubscribe *wsnt__Unsubscribe, _wsnt__UnsubscribeResponse &wsnt__UnsubscribeResponse)
{
    SOAP_EMPTY_HANDLER(wsnt__Unsubscribe, "PullPointSubscription");
}
