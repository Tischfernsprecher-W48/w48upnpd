#include <syslog.h>
#include <stdlib.h>
#include <upnp/ixml.h>
#include <string.h>
#include <time.h>
#include <upnp/upnp.h>
#include <upnp/upnptools.h>
#include <upnp/TimerThread.h>
#include "globals.h"
#include "gatedevice.h"
#include "pmlist.h"
#include "util.h"
#include <stdio.h>

#define ARP_CACHE "/proc/net/arp"
#define LINE_MAX 256

//Definitions for mapping expiration timer thread
static TimerThread gExpirationTimerThread;
static ThreadPool gExpirationThreadPool;

// MUTEX for locking shared state variables whenver they are changed
static ithread_mutex_t DevMutex = PTHREAD_MUTEX_INITIALIZER;

// Main event handler for callbacks from the SDK.  Determine type of event
// and dispatch to the appropriate handler (Note: Get Var Request deprecated
int EventHandler(Upnp_EventType EventType, void *Event, void *Cookie)
{
	switch (EventType)
	{
		case UPNP_EVENT_SUBSCRIPTION_REQUEST:
			HandleSubscriptionRequest((struct Upnp_Subscription_Request *) Event);
			break;
		// -- Deprecated --
		case UPNP_CONTROL_GET_VAR_REQUEST:
			HandleGetVarRequest((struct Upnp_State_Var_Request *) Event);
			break;
		case UPNP_CONTROL_ACTION_REQUEST:
			HandleActionRequest((struct Upnp_Action_Request *) Event);
			break;
		default:
			trace(1, "Error in EventHandler: Unknown event type %d",
						EventType);
	}
	return (0);
}

// Grab our UDN from the Description Document.  This may not be needed, 
// the UDN comes with the request, but we leave this for other device initializations
int StateTableInit(char *descDocUrl)
{
	IXML_Document *ixmlDescDoc;
	int ret;

	if ((ret = UpnpDownloadXmlDoc(descDocUrl, &ixmlDescDoc)) != UPNP_E_SUCCESS)
	{
		syslog(LOG_ERR, "Could not parse description document. Exiting ...");
		UpnpFinish();
		exit(0);
	}

	// Get the UDN from the description document, then free the DescDoc's memory
	gateUDN = GetFirstDocumentItem(ixmlDescDoc, "UDN");
	ixmlDocument_free(ixmlDescDoc);
		
	// Initialize our linked list of port mappings.
	pmlist_Head = pmlist_Current = NULL;
	PortMappingNumberOfEntries = 0;

	return (ret);
}

// Handles subscription request for state variable notifications
int HandleSubscriptionRequest(struct Upnp_Subscription_Request *sr_event)
{
	IXML_Document *propSet = NULL;
	
	ithread_mutex_lock(&DevMutex);

	if (strcmp(sr_event->UDN, gateUDN) == 0)
	{
		// WAN IP Connection Device Notifications
		if (strcmp(sr_event->ServiceId, "urn:any-com:serviceId:fritzbox") == 0)
		{
			GetIpAddressStr(ExternalIPAddress, g_vars.extInterfaceName);
			trace(3, "Received request to subscribe to WANIPConn1");
			UpnpAddToPropertySet(&propSet, "PossibleConnectionTypes","IP_Routed");
			UpnpAddToPropertySet(&propSet, "ConnectionStatus","Connected");
			UpnpAddToPropertySet(&propSet, "ExternalIPAddress", ExternalIPAddress);
			UpnpAddToPropertySet(&propSet, "PortMappingNumberOfEntries","0");
			UpnpAcceptSubscriptionExt(deviceHandle, sr_event->UDN, sr_event->ServiceId,
						  propSet, sr_event->Sid);
			ixmlDocument_free(propSet);
		}
	}
	ithread_mutex_unlock(&DevMutex);
	return(1);
}

int HandleGetVarRequest(struct Upnp_State_Var_Request *gv_request)
{
	// GET VAR REQUEST DEPRECATED FROM UPnP SPECIFICATIONS 
	// Report this in debug and ignore requests.  If anyone experiences problems
	// please let us know.
        trace(3, "Deprecated Get Variable Request received. Ignoring.");
	return 1;
}

int HandleActionRequest(struct Upnp_Action_Request *ca_event)
{
	int result = 0;

	ithread_mutex_lock(&DevMutex);
	
	if (strcmp(ca_event->DevUDN, gateUDN) == 0)
	{
		// Common debugging info, hopefully gets removed soon.
	        trace(3, "ActionName = %s", ca_event->ActionName);
		
		if (strcmp(ca_event->ServiceID, "urn:any-com:serviceId:fritzbox") == 0)
//		if (strcmp(ca_event->ServiceID, "urn:upnp-org:serviceId:WANIPConn1") == 0)
		{
//			if (strcmp(ca_event->ActionName,"GetConnectionTypeInfo") == 0)
//			  result = GetConnectionTypeInfo(ca_event);
//			else if (strcmp(ca_event->ActionName,"GetNATRSIPStatus") == 0)
//			  result = GetNATRSIPStatus(ca_event);
//			else if (strcmp(ca_event->ActionName,"SetConnectionType") == 0)
//			  result = SetConnectionType(ca_event);
//			else if (strcmp(ca_event->ActionName,"RequestConnection") == 0)
//			  result = RequestConnection(ca_event);
//			else if (strcmp(ca_event->ActionName,"AddPortMapping") == 0)
//			  result = AddPortMapping(ca_event);
//			else if (strcmp(ca_event->ActionName,"GetGenericPortMappingEntry") == 0)
//			  result = GetGenericPortMappingEntry(ca_event);
//			else if (strcmp(ca_event->ActionName,"GetSpecificPortMappingEntry") == 0)
//			  result = GetSpecificPortMappingEntry(ca_event);
//			else if (strcmp(ca_event->ActionName,"GetExternalIPAddress") == 0)
//			  result = GetExternalIPAddress(ca_event);
//			else if (strcmp(ca_event->ActionName,"DeletePortMapping") == 0)
//			  result = DeletePortMapping(ca_event);
//			else if (strcmp(ca_event->ActionName,"GetStatusInfo") == 0)
//			  result = GetStatusInfo(ca_event);

// Fritzbox
			if (strcmp(ca_event->ActionName,"SetLogParm") == 0)
			  result = SetLogParm(ca_event);
			else if (strcmp(ca_event->ActionName,"GetMaclist") == 0)
			  result = GetMaclist(ca_event, STATS_MAC_LIST);

			else {
				trace(1, "Invalid Action Request : %s",ca_event->ActionName);
				result = InvalidAction(ca_event);
			}

//			else result = InvalidAction(ca_event);
		}
//		else if (strcmp(ca_event->ServiceID,"urn:upnp-org:serviceId:WANCommonIFC1") == 0)
//		{
//			if (strcmp(ca_event->ActionName,"GetTotalBytesSent") == 0)
//				result = GetTotal(ca_event, STATS_TX_BYTES);
//			else if (strcmp(ca_event->ActionName,"GetTotalBytesReceived") == 0)
//				result = GetTotal(ca_event, STATS_RX_BYTES);
//			else if (strcmp(ca_event->ActionName,"GetTotalPacketsSent") == 0)
//				result = GetTotal(ca_event, STATS_TX_PACKETS);
//			else if (strcmp(ca_event->ActionName,"GetTotalPacketsReceived") == 0)
//				result = GetTotal(ca_event, STATS_RX_PACKETS);
//			else if (strcmp(ca_event->ActionName,"GetCommonLinkProperties") == 0)
//				result = GetCommonLinkProperties(ca_event);
//			else 
//			{
//				trace(1, "Invalid Action Request : %s",ca_event->ActionName);
//				result = InvalidAction(ca_event);
//			}
//		} 
	}
	
	ithread_mutex_unlock(&DevMutex);

	return (result);
}

// Default Action when we receive unknown Action Requests
int InvalidAction(struct Upnp_Action_Request *ca_event) {
        ca_event->ErrCode = 401;
        strcpy(ca_event->ErrStr, "Invalid Action");
        ca_event->ActionResult = NULL;
        return (ca_event->ErrCode);
}

/////////////////////////////////////////////////////////////////////////////////// Fritzbox

int SetLogParm(struct Upnp_Action_Request *ca_event) {
	// Ignore requests
	ca_event->ActionResult = NULL;
	ca_event->ErrCode = UPNP_E_SUCCESS;
	return ca_event->ErrCode;
}



int GetMaclist (struct Upnp_Action_Request *ca_event, stats_t stat) {


    IXML_Document *result;

  FILE *arpCache = fopen(ARP_CACHE, "r");
  if (!arpCache)  {
    perror("Failed to open file \"" ARP_CACHE "\"");
    return 1;
  }

  char line[LINE_MAX] = {0}, *resultStr = NULL;

  size_t memSize = 256u;
  if (fgets(line, LINE_MAX, arpCache) == NULL || (resultStr = malloc(memSize)) == NULL) {
    fclose(arpCache);
    return 1;
  }

    strcpy(resultStr, "<u:GetMaclistResponse xmlns:u=\"urn:schemas-any-com:service:fritzbox:1\">\n<NewMaclist>");
    size_t total = strlen(resultStr);

  while (fgets(line, LINE_MAX, arpCache)) {
    if (strlen(line) > 58u) {
      if (memSize > 256u) strcpy(resultStr + total++, "-");
      memSize += 18u;
      char *ptr = realloc(resultStr, memSize);
      if (ptr == NULL) {
        free(resultStr);
        fclose(arpCache);
        return 1;
      }

      resultStr = ptr;
      strncpy(resultStr + total, line + 41, 17u);
      total += 17u;
    }
  }

  fclose(arpCache);
  strcpy(resultStr + total, "</NewMaclist>\n<NewMaclistChangeCounter>0</NewMaclistChangeCounter>\n</u:GetMaclistResponse>\n");

////////////////

   // Create a IXML_Document from resultStr and return with ca_event
   if ((result = ixmlParseBuffer(resultStr)) != NULL) {
      ca_event->ActionResult = result;
      ca_event->ErrCode = UPNP_E_SUCCESS;
   } else {
      trace(1, "Error parsing Response to GetConnectionTypeinfo: %s", resultStr);
      ca_event->ActionResult = NULL;
      ca_event->ErrCode = 402;
   }
        free(resultStr);
	return(ca_event->ErrCode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



// From sampleutil.c included with libupnp 
char* GetFirstDocumentItem( IN IXML_Document * doc,
                                 IN const char *item )
{
    IXML_NodeList *nodeList = NULL;
    IXML_Node *textNode = NULL;
    IXML_Node *tmpNode = NULL;

    char *ret = NULL;

    nodeList = ixmlDocument_getElementsByTagName( doc, ( char * )item );

    if( nodeList ) {
        if( ( tmpNode = ixmlNodeList_item( nodeList, 0 ) ) ) {
            textNode = ixmlNode_getFirstChild( tmpNode );
       if (textNode != NULL)
       {
      ret = strdup( ixmlNode_getNodeValue( textNode ) );
       }
        }
    }

    if( nodeList )
        ixmlNodeList_free( nodeList );
    return ret;
}

int ExpirationTimerThreadInit(void)
{
  int retVal;
  ThreadPoolAttr attr;
  TPAttrInit( &attr );
  TPAttrSetMaxThreads( &attr, MAX_THREADS );
  TPAttrSetMinThreads( &attr, MIN_THREADS );
  TPAttrSetJobsPerThread( &attr, JOBS_PER_THREAD );
  TPAttrSetIdleTime( &attr, THREAD_IDLE_TIME );

  if( ThreadPoolInit( &gExpirationThreadPool, &attr ) != UPNP_E_SUCCESS ) {
    return UPNP_E_INIT_FAILED;
  }

  if( ( retVal = TimerThreadInit( &gExpirationTimerThread,
				  &gExpirationThreadPool ) ) !=
      UPNP_E_SUCCESS ) {
    return retVal;
  }
  
  return 0;
}

int ExpirationTimerThreadShutdown(void)
{
  return TimerThreadShutdown(&gExpirationTimerThread);
}


void free_expiration_event(expiration_event *event)
{
  if (event->mapping!=NULL)
    event->mapping->expirationEventId = -1;
  free(event);
}

void ExpireMapping(void *input)
{
  char num[5]; // Maximum number of port mapping entries 9999
  IXML_Document *propSet = NULL;
  expiration_event *event = ( expiration_event * ) input;
    
  ithread_mutex_lock(&DevMutex);

  trace(2, "ExpireMapping: Proto:%s Port:%s\n",
		      event->mapping->m_PortMappingProtocol, event->mapping->m_ExternalPort);

  //reset the event id before deleting the mapping so that pmlist_Delete
  //will not call CancelMappingExpiration
  event->mapping->expirationEventId = -1;
  pmlist_Delete(event->mapping);
  
  sprintf(num, "%d", pmlist_Size());
  UpnpAddToPropertySet(&propSet, "PortMappingNumberOfEntries", num);
  UpnpNotifyExt(deviceHandle, event->DevUDN, event->ServiceID, propSet);
  ixmlDocument_free(propSet);
  trace(3, "ExpireMapping: UpnpNotifyExt(deviceHandle,%s,%s,propSet)\n  PortMappingNumberOfEntries: %s",
		      event->DevUDN, event->ServiceID, num);
  
  free_expiration_event(event);
  
  ithread_mutex_unlock(&DevMutex);
}

int ScheduleMappingExpiration(struct portMap *mapping, char *DevUDN, char *ServiceID)
{
  int retVal = 0;
  ThreadPoolJob job;
  expiration_event *event;
  time_t curtime = time(NULL);
	
  if (mapping->m_PortMappingLeaseDuration > 0) {
    mapping->expirationTime = curtime + mapping->m_PortMappingLeaseDuration;
  }
  else {
    //client did not provide a duration, so use the default duration
    if (g_vars.duration==0) {
      return 1; //no default duration set
    }
    else if (g_vars.duration>0) {
      //relative duration
      mapping->expirationTime = curtime+g_vars.duration;
    }
    else { //g_vars.duration < 0
      //absolute daily expiration time
      long int expclock = -1*g_vars.duration;
      struct tm *loctime = localtime(&curtime);
      long int curclock = loctime->tm_hour*3600 + loctime->tm_min*60 + loctime->tm_sec;
      long int diff = expclock-curclock;
      if (diff<60) //if exptime is in less than a minute (or in the past), schedule it in 24 hours instead
	diff += 24*60*60;
      mapping->expirationTime = curtime+diff;
    }
  }

  event = ( expiration_event * ) malloc( sizeof( expiration_event ) );
  if( event == NULL ) {
    return 0;
  }
  event->mapping = mapping;
  if (strlen(DevUDN) < sizeof(event->DevUDN)) strcpy(event->DevUDN, DevUDN);
  else strcpy(event->DevUDN, "");
  if (strlen(ServiceID) < sizeof(event->ServiceID)) strcpy(event->ServiceID, ServiceID);
  else strcpy(event->ServiceID, "");
  
  TPJobInit( &job, ( start_routine ) ExpireMapping, event );
  TPJobSetFreeFunction( &job, ( free_routine ) free_expiration_event );
  if( ( retVal = TimerThreadSchedule( &gExpirationTimerThread,
				      mapping->expirationTime,
				      ABS_SEC, &job, SHORT_TERM,
				      &( event->eventId ) ) )
      != UPNP_E_SUCCESS ) {
    free( event );
    mapping->expirationEventId = -1;
    return 0;
  }
  mapping->expirationEventId = event->eventId;

  trace(3,"ScheduleMappingExpiration: DevUDN: %s ServiceID: %s Proto: %s ExtPort: %s Int: %s.%s at: %s eventId: %d",event->DevUDN,event->ServiceID,mapping->m_PortMappingProtocol, mapping->m_ExternalPort, mapping->m_InternalClient, mapping->m_InternalPort, ctime(&(mapping->expirationTime)), event->eventId);

  return event->eventId;
}

int CancelMappingExpiration(int expirationEventId)
{
  ThreadPoolJob job;
  if (expirationEventId<0)
    return 1;
  trace(3,"CancelMappingExpiration: eventId: %d",expirationEventId);
  if (TimerThreadRemove(&gExpirationTimerThread,expirationEventId,&job)==0) {
    free_expiration_event((expiration_event *)job.arg);
  }
  else {
    trace(1,"  TimerThreadRemove failed!");
  }
  return 1;
}

void DeleteAllPortMappings(void)
{
  IXML_Document *propSet = NULL;

  ithread_mutex_lock(&DevMutex);

  pmlist_FreeList();

  UpnpAddToPropertySet(&propSet, "PortMappingNumberOfEntries", "0");
  UpnpNotifyExt(deviceHandle, gateUDN, "urn:upnp-org:serviceId:WANIPConn1", propSet);
  ixmlDocument_free(propSet);
  trace(2, "DeleteAllPortMappings: UpnpNotifyExt(deviceHandle,%s,%s,propSet)\n  PortMappingNumberOfEntries: %s",
	gateUDN, "urn:upnp-org:serviceId:WANIPConn1", "0");

  ithread_mutex_unlock(&DevMutex);
}
