/**
 *   \file ovfs_onvif_http.h
 *   \brief A Documented file.
 *
 *  Detailed description
 *
 */

#ifndef _OVFS_ONVIF_HTTP_H_
#define _OVFS_ONVIF_HTTP_H_



const char *ONVIF_SCAN_RSP = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope \
xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" \
xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" \
xmlns:wa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" \
xmlns:wd=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" \
xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\" \
xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\">\
<SOAP-ENV:Header>\
<wa:MessageID>uuid:2419d68a-2dd2-21b2-a205-%s</wa:MessageID>  \
<wa:RelatesTo>%s</wa:RelatesTo>\
<wa:To SOAP-ENV:mustUnderstand=\"true\">http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous</wa:To>\
<wa:Action SOAP-ENV:mustUnderstand=\"true\">http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches</wa:Action>\
</SOAP-ENV:Header>\
<SOAP-ENV:Body>\
<wd:ProbeMatches>\
<wd:ProbeMatch> \
<wa:EndpointReference>\
<wa:Address>urn:uuid:2419d68a-2dd2-21b2-a205-%s</wa:Address>\
</wa:EndpointReference>\
<wd:Types>dn:NetworkVideoTransmitter tds:Device</wd:Types>\
<wd:Scopes>onvif://www.onvif.org/Profile/Streaming \
onvif://www.onvif.org/Profile/T \
onvif://www.onvif.org/type/Network_Video_Transmitter \
%s \
onvif://www.onvif.org/hardware/IPC \
onvif://www.onvif.org/name/ \
</wd:Scopes>\
<wd:XAddrs>http://%s:%d/onvif/device_service</wd:XAddrs>\
<wd:MetadataVersion>1</wd:MetadataVersion>\
</wd:ProbeMatch>\
</wd:ProbeMatches>\
</SOAP-ENV:Body>\
</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HELLO_REQ = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope \
xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" \
xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" \
xmlns:wa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" \
xmlns:wd=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" \
xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\" \
xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\">\
<SOAP-ENV:Header>\
<wa:MessageID>uuid:2419d68a-2dd2-21b2-a205-%s</wa:MessageID>  \
<wa:RelatesTo>http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous</wa:RelatesTo>\
<wa:To SOAP-ENV:mustUnderstand=\"true\">urn:schemas-xmlsoap-org:ws:2005:04:discovery</wa:To>\
<wa:Action SOAP-ENV:mustUnderstand=\"true\">http://schemas.xmlsoap.org/ws/2005/04/discovery/Hello</wa:Action>\
</SOAP-ENV:Header>\
<SOAP-ENV:Body>\
<wd:Hello> \
<wa:EndpointReference>\
<wa:Address>urn:uuid:2419d68a-2dd2-21b2-a205-%s</wa:Address>\
</wa:EndpointReference>\
<wd:Types>dn:NetworkVideoTransmitter tds:Device</wd:Types>\
<wd:Scopes>onvif://www.onvif.org/Profile/Streaming \
onvif://www.onvif.org/Profile/T \
onvif://www.onvif.org/type/Network_Video_Transmitter \
%s \
onvif://www.onvif.org/hardware/IPC \
onvif://www.onvif.org/name/ \
</wd:Scopes>\
<wd:XAddrs>http://%s:%d/onvif/device_service</wd:XAddrs>\
<wd:MetadataVersion>1</wd:MetadataVersion>\
</wd:Hello>\
</SOAP-ENV:Body>\
</SOAP-ENV:Envelope>\r\n";



const char *ONVIF_HTTP_XMLNS =
    "xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" \
xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" \
xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" \
xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" \
xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" \
xmlns:wsa5=\"http://www.w3.org/2005/08/addressing\" \
xmlns:wsu=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\" \
xmlns:xenc=\"http://www.w3.org/2001/04/xmlenc#\" \
xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\" \
xmlns:wsse=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd\" \
xmlns:xmime=\"http://tempuri.org/xmime.xsd\" \
xmlns:xmime5=\"http://www.w3.org/2005/05/xmlmime\" \
xmlns:xmime4=\"http://www.w3.org/2004/11/xmlmime\" \
xmlns:xop=\"http://www.w3.org/2004/08/xop/include\" \
xmlns:tt=\"http://www.onvif.org/ver10/schema\" \
xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" \
xmlns:tmd=\"http://www.onvif.org/ver10/deviceIO/wsdl\" \
xmlns:trt=\"http://www.onvif.org/ver10/media/wsdl\" \
xmlns:tr2=\"http://www.onvif.org/ver20/media/wsdl\" \
xmlns:trp=\"http://www.onvif.org/ver10/replay/wsdl\" \
xmlns:tse=\"http://www.onvif.org/ver10/search/wsdl\" \
xmlns:tptz=\"http://www.onvif.org/ver20/ptz/wsdl\" \
xmlns:tptz1=\"http://www.onvif.org/ver10/ptz/wsdl\" \
xmlns:timg=\"http://www.onvif.org/ver20/imaging/wsdl\" \
xmlns:timg1=\"http://www.onvif.org/ver10/imaging/wsdl\" \
xmlns:tev=\"http://www.onvif.org/ver10/events/wsdl\" \
xmlns:tan=\"http://www.onvif.org/ver20/analytics/wsdl\" \
xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\" \
xmlns:extXsd=\"http://www.onvifext.org/ver10/extFunctions/wsdl\" \
xmlns:tns1=\"http://www.onvif.org/ver10/topics\" \
xmlns:ter=\"http://www.onvif.org/ver10/error\" \
xmlns:tanae=\"http://www.onvif.org/ver20/analytics/wsdl/AnalyticsEngineBinding\" \
xmlns:tanre=\"http://www.onvif.org/ver20/analytics/wsdl/RuleEngineBinding\"\
>";


const char *ONVIF_HTTP_401_HEAD = "HTTP/1.1 401 Unauthorized\r\n\
WWW-Authenticate: Digest realm=\"abc@test.com\", qop=\"auth\", nonce=\"%s\", opaque=\"%s\"\r\n\
Server: onvifserver\r\nContent-Type: application/soap+xml; charset=utf-8\r\n\
Content-Length: %d\r\nConnection: close\r\n\r\n";

const char *ONVIF_HTTP_401_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s<SOAP-ENV:Body>\n\
\t\t<SOAP-ENV:Fault>\n\
\t\t\t<SOAP-ENV:Code>\n\
\t\t\t\t<SOAP-ENV:Value>SOAP-ENV:Sender</SOAP-ENV:Value>\n\
\t\t\t</SOAP-ENV:Code>\n\
\t\t\t<SOAP-ENV:Reason>\n\
\t\t\t\t<SOAP-ENV:Text xml:lang=\"en\">Error 401: HTTP 401 Unauthorized</SOAP-ENV:Text>\n\
\t\t\t</SOAP-ENV:Reason>\n\
\t\t</SOAP-ENV:Fault>\n\
\t</SOAP-ENV:Body>\n\
</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_400_HEAD = "HTTP/1.1 400 Bad Request\r\n\
Server: onvifserver\r\nContent-Type: application/soap+xml; charset=utf-8\r\n\
Content-Length: %d\r\nConnection: close\r\n\r\n";

const char *ONVIF_HTTP_400_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\
\n\t<SOAP-ENV:Header></SOAP-ENV:Header>\
<SOAP-ENV:Body>\n\
\t\t<SOAP-ENV:Fault>\n\
\t\t\t<SOAP-ENV:Code>\n\
\t\t\t\t<SOAP-ENV:Value>SOAP-ENV:Sender</SOAP-ENV:Value>\n\
\t\t\t\t<SOAP-ENV:Subcode>\n\
\t\t\t\t\t<SOAP-ENV:Value>ter:NotAuthorized</SOAP-ENV:Value>\n\
\t\t\t\t</SOAP-ENV:Subcode>\n\
\t\t\t</SOAP-ENV:Code>\n\
\t\t\t<SOAP-ENV:Reason>\n\
\t\t\t\t<SOAP-ENV:Text xml:lang=\"en\">The security token could not be authenticated or authorized</SOAP-ENV:Text>\n\
\t\t\t</SOAP-ENV:Reason>\n\
\t\t</SOAP-ENV:Fault>\n\
\t</SOAP-ENV:Body>\n\
</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_200_HEAD = "HTTP/1.1 200 OK\r\n\
Server: onvifserver\r\nContent-Type: application/soap+xml; charset=utf-8\r\n\
Content-Length: %d\r\nConnection: close\r\n\r\n";


const char *ONVIF_HTTP_DEVICE_INFO_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\t<SOAP-ENV:Body>\n\
\t\t<tds:GetDeviceInformationResponse>\n\
\t\t\t<tds:Manufacturer>%s</tds:Manufacturer>\n\
\t\t\t<tds:Model>%s</tds:Model>\n\
\t\t\t<tds:FirmwareVersion>%s</tds:FirmwareVersion>\n\
\t\t\t<tds:SerialNumber>%s</tds:SerialNumber>\n\
\t\t\t<tds:HardwareId>%s</tds:HardwareId>\n\
\t\t</tds:GetDeviceInformationResponse>\n\
\t</SOAP-ENV:Body>\n\
</SOAP-ENV:Envelope>\r\n";


const char *ONVIF_HTTP_DATE_TIME_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t<tds:GetSystemDateAndTimeResponse>\n\t\t\t\
<tds:SystemDateAndTime>\n\t\t\t\t\
<tt:DateTimeType>%s</tt:DateTimeType>\n\t\t\t\t\
<tt:DaylightSavings>%s</tt:DaylightSavings>\n\t\t\t\t\
<tt:TimeZone>\n\t\t\t\t\t\
<tt:TZ>%s</tt:TZ>\n\t\t\t\t\
</tt:TimeZone>\n\t\t\t\t\
<tt:UTCDateTime>\n\t\t\t\t\t\
<tt:Time>\n\t\t\t\t\t\t\
<tt:Hour>%d</tt:Hour>\n\t\t\t\t\t\t\
<tt:Minute>%d</tt:Minute>\n\t\t\t\t\t\t\
<tt:Second>%d</tt:Second>\n\t\t\t\t\t\
</tt:Time>\n\t\t\t\t\t\
<tt:Date>\n\t\t\t\t\t\t\
<tt:Year>%d</tt:Year>\n\t\t\t\t\t\t\
<tt:Month>%d</tt:Month>\n\t\t\t\t\t\t\
<tt:Day>%d</tt:Day>\n\t\t\t\t\t\
</tt:Date>\n\t\t\t\t\
</tt:UTCDateTime>\n\t\t\t\t\
<tt:LocalDateTime>\n\t\t\t\t\t\
<tt:Time>\n\t\t\t\t\t\t\
<tt:Hour>%d</tt:Hour>\n\t\t\t\t\t\t\
<tt:Minute>%d</tt:Minute>\n\t\t\t\t\t\t\
<tt:Second>%d</tt:Second>\n\t\t\t\t\t\
</tt:Time>\n\t\t\t\t\t\
<tt:Date>\n\t\t\t\t\t\t\
<tt:Year>%d</tt:Year>\n\t\t\t\t\t\t\
<tt:Month>%d</tt:Month>\n\t\t\t\t\t\t\
<tt:Day>%d</tt:Day>\n\t\t\t\t\t\
</tt:Date>\n\t\t\t\t\
</tt:LocalDateTime>\n\t\t\t\
</tds:SystemDateAndTime>\n\t\t\
</tds:GetSystemDateAndTimeResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_Service_Capabilities_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<SOAP-ENV:Envelope \
xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" \
xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" \
xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" \
xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" \
xmlns:wsc=\"http://docs.oasis-open.org/ws-sx/ws-secureconversation/200512\" \
xmlns:wsse=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd\" \
xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" \
xmlns:wsa5=\"http://www.w3.org/2005/08/addressing\" \
xmlns:xmime=\"http://tempuri.org/xmime.xsd\" \
xmlns:xop=\"http://www.w3.org/2004/08/xop/include\" \
xmlns:tt=\"http://www.onvif.org/ver10/schema\" \
xmlns:wsbf2=\"http://docs.oasis-open.org/wsrf/bf-2\" \
xmlns:wstop=\"http://docs.oasis-open.org/wsn/t-1\" \
xmlns:wsr2=\"http://docs.oasis-open.org/wsrf/r-2\" \
xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\" \
xmlns:tetsm=\"http://www.onvif.org/ver10/events/wsdl/SubscriptionManagerBinding\" \
xmlns:tr2=\"http://www.onvif.org/ver20/media/wsdl\" \
xmlns:trt=\"http://www.onvif.org/ver10/media/wsdl\" \
xmlns:ter=\"http://www.onvif.org/ver10/error\">\
<SOAP-ENV:Body>\
<tr2:GetServiceCapabilitiesResponse>\
<tr2:Capabilities SnapshotUri=\"true\" Rotation=\"false\" VideoSourceMode=\"true\" OSD=\"true\" TemporaryOSDText=\"false\" Mask=\"false\" SourceMask=\"false\">\
<tr2:ProfileCapabilities MaximumNumberOfProfiles=\"64\" ConfigurationsSupported=\"VideoSource AudioSource VideoEncoder AudioEncoder AudioOutput AudioDecoder PTZ Analytics\">\
</tr2:ProfileCapabilities><tr2:StreamingCapabilities RTSPStreaming=\"true\" RTPMulticast=\"true\" RTP_RTSP_TCP=\"true\" NonAggregateControl=\"true\" AutoStartMulticast=\"true\">\
</tr2:StreamingCapabilities>\
</tr2:Capabilities>\
</tr2:GetServiceCapabilitiesResponse>\
</SOAP-ENV:Body></SOAP-ENV:Envelope>";

/*
<tds:Network>\n\t\t\t\t\
<tt:IPFilter>false</tt:IPFilter>\n\t\t\t\t\
<tt:ZeroConfiguration>false</tt:ZeroConfiguration>\n\t\t\t\t\
<tt:IPVersion6>false</tt:IPVersion6>\n\t\t\t\t\
<tt:DynDNS>false</tt:DynDNS>\n\t\t\t\t\
<tt:Dot11Configuration>false</tt:Dot11Configuration>\n\t\t\t\t\
<tt:HostnameFromDHCP>false</tt:HostnameFromDHCP>\n\t\t\t\t\
<tt:TP>false</tt:TP>\n\t\t\t\t\
<tt:DHCPv6>false</tt:DHCPv6>\n\t\t\t\t\
</tds:Network>\n\t\t\t\
<tds:Security>\n\t\t\t\t\t\
<tt:TLS1.0>false</tt:TLS1.0>\n\t\t\t\t\t\
<tt:TLS1.1>false</tt:TLS1.1>\n\t\t\t\t\t\
<tt:TLS1.2>false</tt:TLS1.2>\n\t\t\t\t\t\
<tt:OnboardKeyGeneration>false</tt:OnboardKeyGeneration>\n\t\t\t\t\t\
<tt:AccessPolicyConfig>false</tt:AccessPolicyConfig>\n\t\t\t\t\t\
<tt:DefaultAccessPolicy>false</tt:DefaultAccessPolicy>\n\t\t\t\t\t\
<tt:Dot1X>false</tt:Dot1X>\n\t\t\t\t\t\
<tt:RemoteUserHandling>false</tt:RemoteUserHandling>\n\t\t\t\t\t\
<tt:X.509Token>false</tt:X.509Token>\n\t\t\t\t\t\
<tt:SAMLToken>false</tt:SAMLToken>\n\t\t\t\t\t\
<tt:KerberosToken>false</tt:KerberosToken>\n\t\t\t\t\t\
<tt:UsernameToken>true</tt:UsernameToken>\n\t\t\t\t\t\
<tt:HttpDigest>true</tt:HttpDigest>\n\t\t\t\t\t\
<tt:RELToken>false</tt:RELToken>\n\t\t\t\t\t\
<tt:SupportedEAPMethods>0</tt:SupportedEAPMethods>\n\t\t\t\t\t\
<tt:MaxUsers>32</tt:MaxUsers>\n\t\t\t\t\t\
<tt:MaxUserNameLength>32</tt:MaxUserNameLength>\n\t\t\t\t\t\
<tt:MaxPasswordLength>16</tt:MaxPasswordLength>\n\t\t\t\t\t\
</tds:Security>\n\t\t\t\t\
<tds:System>\n\t\t\t\t\t\
<tt:DiscoveryResolve>false</tt:DiscoveryResolve>\n\t\t\t\t\t\
<tt:DiscoveryBye>true</tt:DiscoveryBye>\n\t\t\t\t\t\
<tt:RemoteDiscovery>false</tt:RemoteDiscovery>\n\t\t\t\t\t\
<tt:SystemBackup>false</tt:SystemBackup>\n\t\t\t\t\t\
<tt:SystemLogging>false</tt:SystemLogging>\n\t\t\t\t\t\
<tt:FirmwareUpgrade>true</tt:FirmwareUpgrade>\n\t\t\t\t\t\
</tds:System>\n\t\t\t\t\

*/

const char *ONVIF_HTTP_Create_OSD_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Header></SOAP-ENV:Header>\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:CreateOSDResponse>\n\t\t\t\
<%s:OSDToken>%s</%s:OSDToken>\n\t\t\
</%s:CreateOSDResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_Delete_OSD_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Header></SOAP-ENV:Header>\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:DeleteOSDResponse></%s:DeleteOSDResponse>\n\t \
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_DNS_BODY = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tds:GetDNSResponse>\n\t\t\t\
<tds:DNSInformation>\n\t\t\t\t\
<tt:FromDHCP>false</tt:FromDHCP>\n\t\t\t\t\
<tt:DNSManual>\n\t\t\t\t\t\
<tt:Type>IPv4</tt:Type>\n\t\t\t\t\t\
<tt:IPv4Address>%s</tt:IPv4Address>\n\t\t\t\t\
</tt:DNSManual>\n\t\t\t\t\
<tt:DNSManual>\n\t\t\t\t\t\
<tt:Type>IPv4</tt:Type>\n\t\t\t\t\t\
<tt:IPv4Address>%s</tt:IPv4Address>\n\t\t\t\t\
</tt:DNSManual>\n\t\t\t\
</tds:DNSInformation>\n\t\t\
</tds:GetDNSResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_NETWORK_IF_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tds:GetNetworkInterfacesResponse>\n\t\t\t\
<tds:NetworkInterfaces token=\"%s\">\n\t\t\t\t\
<tt:Enabled>true</tt:Enabled>\n\t\t\t\t\
<tt:Info>\n\t\t\t\t\t\
<tt:Name>%s</tt:Name>\n\t\t\t\t\t\
<tt:HwAddress>%s</tt:HwAddress>\n\t\t\t\t\t\
<tt:MTU>1500</tt:MTU>\n\t\t\t\t\
</tt:Info>\n\t\t\t\t\
<tt:Link>\n\t\t\t\t\t\
<tt:AdminSettings>\n\t\t\t\t\t\t\
<tt:AutoNegotiation>false</tt:AutoNegotiation>\n\t\t\t\t\t\t\
<tt:Speed>100</tt:Speed>\n\t\t\t\t\t\t\
<tt:Duplex>Full</tt:Duplex>\n\t\t\t\t\t\
</tt:AdminSettings>\n\t\t\t\t\t\
<tt:OperSettings>\n\t\t\t\t\t\t\
<tt:AutoNegotiation>false</tt:AutoNegotiation>\n\t\t\t\t\t\t\
<tt:Speed>100</tt:Speed>\n\t\t\t\t\t\t\
<tt:Duplex>Full</tt:Duplex>\n\t\t\t\t\t\
</tt:OperSettings>\n\t\t\t\t\t\
<tt:InterfaceType>6</tt:InterfaceType>\n\t\t\t\t\
</tt:Link>\n\t\t\t\t\
<tt:IPv4>\n\t\t\t\t\t\
<tt:Enabled>true</tt:Enabled>\n\t\t\t\t\t\
<tt:Config>\n\t\t\t\t\t\t\
<tt:Manual>\n\t\t\t\t\t\t\t\
<tt:Address>%s</tt:Address>\n\t\t\t\t\t\t\t\
<tt:PrefixLength>%d</tt:PrefixLength>\n\t\t\t\t\t\t\
</tt:Manual>\n\t\t\t\t\t\t\
<tt:DHCP>%s</tt:DHCP>\n\t\t\t\t\t\
</tt:Config>\n\t\t\t\t\
</tt:IPv4>\n\t\t\t\
</tds:NetworkInterfaces>\n\t\t\
</tds:GetNetworkInterfacesResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_SCOPE_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tds:GetScopesResponse>\n\t\t\t\
<tds:Scopes>\n\t\t\t\t\
<tt:ScopeDef>Fixed</tt:ScopeDef>\n\t\t\t\t\
<tt:ScopeItem>onvif://www.onvif.org/Profile/Streaming</tt:ScopeItem>\n\t\t\t\
</tds:Scopes>\n\t\t\t\
<tds:Scopes>\n\t\t\t\t\
<tt:ScopeDef>Fixed</tt:ScopeDef>\n\t\t\t\t\
<tt:ScopeItem>onvif://www.onvif.org/type/Network_Video_Transmitter</tt:ScopeItem>\n\t\t\t\
</tds:Scopes>\n\t\t\t\
<tds:Scopes>\n\t\t\t\t\
<tt:ScopeDef>Fixed</tt:ScopeDef>\n\t\t\t\t\
<tt:ScopeItem>onvif://www.onvif.org/type/video_encoder</tt:ScopeItem>\n\t\t\t\
</tds:Scopes>\n\t\t\t\
<tds:Scopes>\n\t\t\t\t\
<tt:ScopeDef>Fixed</tt:ScopeDef>\n\t\t\t\t\
<tt:ScopeItem>onvif://www.onvif.org/type/audio_encoder</tt:ScopeItem>\n\t\t\t\
</tds:Scopes>\n\t\t\t\
<tds:Scopes>\n\t\t\t\t\
<tt:ScopeDef>Configurable</tt:ScopeDef>\n\t\t\t\t\
<tt:ScopeItem>onvif://www.onvif.org/location/%s</tt:ScopeItem>\n\t\t\t\
</tds:Scopes>\n\t\t\t\
<tds:Scopes>\n\t\t\t\t\
<tt:ScopeDef>Configurable</tt:ScopeDef>\n\t\t\t\t\
<tt:ScopeItem>onvif://www.onvif.org/location/%s</tt:ScopeItem>\n\t\t\t\
</tds:Scopes>\n\t\t\t\
<tds:Scopes>\n\t\t\t\t\
<tt:ScopeDef>Configurable</tt:ScopeDef>\n\t\t\t\t\
<tt:ScopeItem>onvif://www.onvif.org/hardware/IPC</tt:ScopeItem>\n\t\t\t\
</tds:Scopes>\n\t\t\t\
<tds:Scopes>\n\t\t\t\t\
<tt:ScopeDef>Configurable</tt:ScopeDef>\n\t\t\t\t\
<tt:ScopeItem>onvif://www.onvif.org/name/%s</tt:ScopeItem>\n\t\t\t\
</tds:Scopes>\n\t\t\
</tds:GetScopesResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_SET_SCOPE_BODY =
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
	<SOAP-ENV:Envelope %s\n\t\
	<SOAP-ENV:Body>\n\t\t\
	<tds:SetScopesResponse>\n\t\t\
	</tds:SetScopesResponse>\n\t\
	</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";


const char *ONVIF_HTTP_CAPABILITIES_EXT_GPS =
    "<extXsd:extFunctions>\n\t\t\t\t\t\t\
<extXsd:XAddr>http://%s:%d/onvif/extFunctions</extXsd:XAddr>\n\t\t\t\t\t\t\
<extXsd:recvAnalogGPS>true</extXsd:recvAnalogGPS>\n\t\t\t\t\t\t\
<extXsd:recvStationInfo>true</extXsd:recvStationInfo>\n\t\t\t\t\t\
</extXsd:extFunctions>\n\t\t\t\t\t\
";

const char *ONVIF_HTTP_CAPABILITIES_EXT =
    "<extXsd:extCapabilities>\n\t\t\t\t\t\t\
<extXsd:XAddr>http://%s:%d/onvif/onvif_ext</extXsd:XAddr>\n\t\t\t\t\t\t\
<extXsd:IOInputSupport>false</extXsd:IOInputSupport>\n\t\t\t\t\t\t\
<extXsd:PrivacyMaskSupport>true</extXsd:PrivacyMaskSupport>\n\t\t\t\t\t\t\
<extXsd:PTZ3DZoomSupport>false</extXsd:PTZ3DZoomSupport>\n\t\t\t\t\t\t\
<extXsd:PTZPatternSupport>false</extXsd:PTZPatternSupport>\n\t\t\t\t\t\t\
<extXsd:Language>1</extXsd:Language>\n\t\t\t\t\t\t\
</extXsd:extCapabilities>\n\t\t\t\t\t\
";

const char *ONVIF_HTTP_CAPABILITIES_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tds:GetCapabilitiesResponse>\n\t\t\t\
<tds:Capabilities>\n\t\t\t\t\
<tt:Analytics>\n\t\t\t\t\t\
<tt:XAddr>http://%s:%d/onvif/Analytics_service</tt:XAddr>\n\t\t\t\t\t\
<tt:RuleSupport>true</tt:RuleSupport>\n\t\t\t\t\t\
<tt:AnalyticsModuleSupport>true</tt:AnalyticsModuleSupport>\n\t\t\t\t\
</tt:Analytics>\n\t\t\t\t\
<tt:Device>\n\t\t\t\t\t\
<tt:XAddr>http://%s:%d/onvif/Device_service</tt:XAddr>\n\t\t\t\t\t\
<tt:Network>\n\t\t\t\t\t\t\
<tt:IPFilter>false</tt:IPFilter>\n\t\t\t\t\t\t\
<tt:ZeroConfiguration>false</tt:ZeroConfiguration>\n\t\t\t\t\t\t\
<tt:IPVersion6>false</tt:IPVersion6>\n\t\t\t\t\t\t\
<tt:DynDNS>false</tt:DynDNS>\n\t\t\t\t\t\
</tt:Network>\n\t\t\t\t\t\
<tt:System>\n\t\t\t\t\t\t\
<tt:DiscoveryResolve>false</tt:DiscoveryResolve>\n\t\t\t\t\t\t\
<tt:DiscoveryBye>true</tt:DiscoveryBye>\n\t\t\t\t\t\t\
<tt:RemoteDiscovery>false</tt:RemoteDiscovery>\n\t\t\t\t\t\t\
<tt:SystemBackup>false</tt:SystemBackup>\n\t\t\t\t\t\t\
<tt:SystemLogging>false</tt:SystemLogging>\n\t\t\t\t\t\t\
<tt:FirmwareUpgrade>true</tt:FirmwareUpgrade>\n\t\t\t\t\t\t\
<tt:SupportedVersions>\n\t\t\t\t\t\t\t\
<tt:Major>2</tt:Major>\n\t\t\t\t\t\t\t\
<tt:Minor>0</tt:Minor>\n\t\t\t\t\t\t\
</tt:SupportedVersions>\n\t\t\t\t\t\t\
<tt:SupportedVersions>\n\t\t\t\t\t\t\t\
<tt:Major>2</tt:Major>\n\t\t\t\t\t\t\t\
<tt:Minor>11</tt:Minor>\n\t\t\t\t\t\t\
</tt:SupportedVersions>\n\t\t\t\t\t\t\
<tt:SupportedVersions>\n\t\t\t\t\t\t\t\
<tt:Major>2</tt:Major>\n\t\t\t\t\t\t\t\
<tt:Minor>21</tt:Minor>\n\t\t\t\t\t\t\
</tt:SupportedVersions>\n\t\t\t\t\t\t\
<tt:SupportedVersions>\n\t\t\t\t\t\t\t\
<tt:Major>2</tt:Major>\n\t\t\t\t\t\t\t\
<tt:Minor>40</tt:Minor>\n\t\t\t\t\t\t\
</tt:SupportedVersions>\n\t\t\t\t\t\t\
<tt:SupportedVersions>\n\t\t\t\t\t\t\t\
<tt:Major>2</tt:Major>\n\t\t\t\t\t\t\t\
<tt:Minor>50</tt:Minor>\n\t\t\t\t\t\t\
</tt:SupportedVersions>\n\t\t\t\t\t\t\
<tt:SupportedVersions>\n\t\t\t\t\t\t\t\
<tt:Major>2</tt:Major>\n\t\t\t\t\t\t\t\
<tt:Minor>60</tt:Minor>\n\t\t\t\t\t\t\
</tt:SupportedVersions>\n\t\t\t\t\t\t\
<tt:SupportedVersions>\n\t\t\t\t\t\t\t\
<tt:Major>16</tt:Major>\n\t\t\t\t\t\t\t\
<tt:Minor>6</tt:Minor>\n\t\t\t\t\t\t\
</tt:SupportedVersions>\n\t\t\t\t\t\t\
<tt:SupportedVersions>\n\t\t\t\t\t\t\t\
<tt:Major>16</tt:Major>\n\t\t\t\t\t\t\t\
<tt:Minor>12</tt:Minor>\n\t\t\t\t\t\t\
</tt:SupportedVersions>\n\t\t\t\t\t\t\
<tt:SupportedVersions>\n\t\t\t\t\t\t\t\
<tt:Major>17</tt:Major>\n\t\t\t\t\t\t\t\
<tt:Minor>6</tt:Minor>\n\t\t\t\t\t\t\
</tt:SupportedVersions>\n\t\t\t\t\t\
<tt:SupportedVersions>\n\t\t\t\t\t\t\t\
<tt:Major>17</tt:Major>\n\t\t\t\t\t\t\t\
<tt:Minor>12</tt:Minor>\n\t\t\t\t\t\t\
</tt:SupportedVersions>\n\t\t\t\t\t\
</tt:System>\n\t\t\t\t\t\
<tt:IO>\n\t\t\t\t\t\t\
<tt:InputConnectors>1</tt:InputConnectors>\n\t\t\t\t\t\t\
<tt:RelayOutputs>1</tt:RelayOutputs>\n\t\t\t\t\t\
</tt:IO>\n\t\t\t\t\t\
<tt:Security>\n\t\t\t\t\t\t\
<tt:TLS1.1>false</tt:TLS1.1>\n\t\t\t\t\t\t\
<tt:TLS1.2>false</tt:TLS1.2>\n\t\t\t\t\t\t\
<tt:OnboardKeyGeneration>false</tt:OnboardKeyGeneration>\n\t\t\t\t\t\t\
<tt:AccessPolicyConfig>false</tt:AccessPolicyConfig>\n\t\t\t\t\t\t\
<tt:X.509Token>false</tt:X.509Token>\n\t\t\t\t\t\t\
<tt:SAMLToken>false</tt:SAMLToken>\n\t\t\t\t\t\t\
<tt:KerberosToken>false</tt:KerberosToken>\n\t\t\t\t\t\t\
<tt:RELToken>false</tt:RELToken>\n\t\t\t\t\t\t\
<tt:Extension>\n\t\t\t\t\t\t\t\
<tt:TLS1.0>false</tt:TLS1.0>\n\t\t\t\t\t\t\t\
<tt:Extension>\n\t\t\t\t\t\t\t\t\
<tt:Dot1X>false</tt:Dot1X>\n\t\t\t\t\t\t\t\t\
<tt:SupportedEAPMethod>0</tt:SupportedEAPMethod>\n\t\t\t\t\t\t\t\t\
<tt:RemoteUserHandling>false</tt:RemoteUserHandling>\n\t\t\t\t\t\t\t\
</tt:Extension>\n\t\t\t\t\t\t\
</tt:Extension>\n\t\t\t\t\t\
</tt:Security>\n\t\t\t\t\
</tt:Device>\n\t\t\t\t\
<tt:Events>\n\t\t\t\t\t\
<tt:XAddr>http://%s:%d/onvif/Events</tt:XAddr>\n\t\t\t\t\t\
<tt:WSSubscriptionPolicySupport>true</tt:WSSubscriptionPolicySupport>\n\t\t\t\t\t\
<tt:WSPullPointSupport>true</tt:WSPullPointSupport>\n\t\t\t\t\t\
<tt:WSPausableSubscriptionManagerInterfaceSupport>false</tt:WSPausableSubscriptionManagerInterfaceSupport>\n\t\t\t\t\
</tt:Events>\n\t\t\t\t\
<tt:Imaging>\n\t\t\t\t\t\
<tt:XAddr>http://%s:%d/onvif/Imaging</tt:XAddr>\n\t\t\t\t\
</tt:Imaging>\n\t\t\t\t\
<tt:Media>\n\t\t\t\t\t\
<tt:XAddr>http://%s:%d/onvif/Media</tt:XAddr>\n\t\t\t\t\t\
<tt:StreamingCapabilities>\n\t\t\t\t\t\t\
<tt:RTPMulticast>false</tt:RTPMulticast>\n\t\t\t\t\t\t\
<tt:RTP_TCP>false</tt:RTP_TCP>\n\t\t\t\t\t\t\
<tt:RTP_RTSP_TCP>true</tt:RTP_RTSP_TCP>\n\t\t\t\t\t\
</tt:StreamingCapabilities>\n\t\t\t\t\
</tt:Media>\n\t\t\t\t\
<tt:Media>\n\t\t\t\t\t\
<tt:XAddr>http://%s:%d/onvif/Media20</tt:XAddr>\n\t\t\t\t\t\
<tt:StreamingCapabilities>\n\t\t\t\t\t\t\
<tt:RTPMulticast>false</tt:RTPMulticast>\n\t\t\t\t\t\t\
<tt:RTP_TCP>false</tt:RTP_TCP>\n\t\t\t\t\t\t\
<tt:RTP_RTSP_TCP>true</tt:RTP_RTSP_TCP>\n\t\t\t\t\t\
</tt:StreamingCapabilities>\n\t\t\t\t\
</tt:Media>\n\t\t\t\t\
<tt:PTZ>\n\t\t\t\t\t\t\
<tt:XAddr>http://%s:%d/onvif/ptz</tt:XAddr>\n\t\t\t\t\t\t\
</tt:PTZ>\n\t\t\t\t\t\t\
<tt:Extension>\n\t\t\t\t\t\
%s\
<tt:DeviceIO>\n\t\t\t\t\t\t\
<tt:XAddr>http://%s:%d/onvif/DeviceIO</tt:XAddr>\n\t\t\t\t\t\t\
<tt:VideoSources>1</tt:VideoSources>\n\t\t\t\t\t\t\
<tt:VideoOutputs>1</tt:VideoOutputs>\n\t\t\t\t\t\t\
<tt:AudioSources>1</tt:AudioSources>\n\t\t\t\t\t\t\
<tt:AudioOutputs>1</tt:AudioOutputs>\n\t\t\t\t\t\t\
<tt:RelayOutputs>1</tt:RelayOutputs>\n\t\t\t\t\t\
</tt:DeviceIO>\n\t\t\t\t\
</tt:Extension>\n\t\t\t\
</tds:Capabilities>\n\t\t\
</tds:GetCapabilitiesResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_PROFILE_2_CONTENT =
    "<tr2:Profiles token=\"Profile2_1_1_%d\" fixed=\"true\">\n\t\t\t\t\
<tr2:Name>D01_CH01_%s</tr2:Name>\n\t\t\t\t\
<tr2:Configurations>\n\t\t\t\t\t\
\
<tr2:VideoSource token=\"VideoSourceConfig_1_1\">\n\t\t\t\t\t\t\
<tt:Name>VideoSourceConfig1_1</tt:Name>\n\t\t\t\t\t\t\
<tt:UseCount>6</tt:UseCount>\n\t\t\t\t\t\t\
<tt:SourceToken>VideoSource_1</tt:SourceToken>\n\t\t\t\t\t\t\
<tt:Bounds x=\"0\" y=\"0\" width=\"%d\" height=\"%d\"></tt:Bounds>\n\t\t\t\t\t\
</tr2:VideoSource>\n\t\t\t\t\t\
\
<tr2:AudioSource token=\"AudioSourceConfig_1\">\n\t\t\t\t\t\t\
<tt:Name>AudioSourceConfig1</tt:Name>\n\t\t\t\t\t\t\
<tt:UseCount>6</tt:UseCount>\n\t\t\t\t\t\t\
<tt:SourceToken>AudioSource_1</tt:SourceToken>\n\t\t\t\t\t\
</tr2:AudioSource>\n\t\t\t\t\t\
\
<tr2:VideoEncoder token=\"VideoEncoder2Token_1_1_%d\" GovLength=\"%d\">\n\t\t\t\t\t\t\
<tt:Name>%sStream</tt:Name>\n\t\t\t\t\t\t\
<tt:UseCount>1</tt:UseCount>\n\t\t\t\t\t\t\
<tt:Encoding>%s</tt:Encoding>\n\t\t\t\t\t\t\
<tt:Resolution>\n\t\t\t\t\t\t\t\
<tt:Width>%d</tt:Width>\n\t\t\t\t\t\t\t\
<tt:Height>%d</tt:Height>\n\t\t\t\t\t\t\
</tt:Resolution>\n\t\t\t\t\t\t\
<tt:RateControl ConstantBitRate=\"%s\">\n\t\t\t\t\t\t\t\
<tt:FrameRateLimit>%d</tt:FrameRateLimit>\n\t\t\t\t\t\t\t\
<tt:BitrateLimit>%d</tt:BitrateLimit>\n\t\t\t\t\t\t\
</tt:RateControl>\n\t\t\t\t\t\t\
<tt:Multicast>\n\t\t\t\t\t\t\t\
<tt:Address>\n\t\t\t\t\t\t\t\t\
<tt:Type>IPv4</tt:Type>\n\t\t\t\t\t\t\t\t\
<tt:IPv4Address>238.255.0.2</tt:IPv4Address>\n\t\t\t\t\t\t\t\
</tt:Address>\n\t\t\t\t\t\t\t\
<tt:Port>28080</tt:Port>\n\t\t\t\t\t\t\t\
<tt:TTL>255</tt:TTL>\n\t\t\t\t\t\t\t\
<tt:AutoStart>false</tt:AutoStart>\n\t\t\t\t\t\t\
</tt:Multicast>\n\t\t\t\t\t\t\
<tt:Quality>%d</tt:Quality>\n\t\t\t\t\t\
</tr2:VideoEncoder>\n\t\t\t\t\t\
\
<tr2:AudioEncoder token=\"AudioEncoder2Token_1\">\n\t\t\t\t\t\t\
<tt:Name>FirstAudio</tt:Name>\n\t\t\t\t\t\t\
<tt:UseCount>3</tt:UseCount>\n\t\t\t\t\t\t\
<tt:Encoding>PCMU</tt:Encoding>\n\t\t\t\t\t\t\
<tt:Bitrate>64</tt:Bitrate>\n\t\t\t\t\t\t\
<tt:SampleRate>8</tt:SampleRate>\n\t\t\t\t\t\
</tr2:AudioEncoder>\n\t\t\t\t\t\
\
<tr2:Analytics token=\"VideoAnalytics_1\">\n\t\t\t\t\t\t\
<tt:Name>VideoAnalytics1</tt:Name>\n\t\t\t\t\t\t\
<tt:UseCount>6</tt:UseCount>\n\t\t\t\t\t\t\
<tt:AnalyticsEngineConfiguration>\n\t\t\t\t\t\t\t\
<tt:AnalyticsModule Type=\"tt:CellMotionEngine\" Name=\"MyCellMotionModule\">\n\t\t\t\t\t\t\t\t\
<tt:Parameters>\n\t\t\t\t\t\t\t\t\t\
<tt:SimpleItem Name=\"Sensitivity\" Value=\"%d\"></tt:SimpleItem>\n\t\t\t\t\t\t\t\t\t\
<tt:ElementItem Name=\"Layout\"><tt:CellLayout Columns=\"22\" Rows=\"18\"><tt:Transformation><tt:Translate x=\"-1.000000\" y=\"-1.000000\"/><tt:Scale x=\"0.045455\" y=\"-0.055556\"/></tt:Transformation></tt:CellLayout></tt:ElementItem>\n\t\t\t\t\t\t\t\t\
</tt:Parameters>\n\t\t\t\t\t\t\t\
</tt:AnalyticsModule>\n\t\t\t\t\t\t\
</tt:AnalyticsEngineConfiguration>\n\t\t\t\t\t\t\
<tt:RuleEngineConfiguration>\n\t\t\t\t\t\t\t\
<tt:Rule Type=\"tt:CellMotionDetector\" Name=\"MyCellMotionDetectorRule\">\n\t\t\t\t\t\t\t\t\
<tt:Parameters>\n\t\t\t\t\t\t\t\t\t\
<tt:SimpleItem Name=\"MinCount\" Value=\"4\"></tt:SimpleItem>\n\t\t\t\t\t\t\t\t\t\
<tt:SimpleItem Name=\"AlarmOnDelay\" Value=\"1000\"></tt:SimpleItem>\n\t\t\t\t\t\t\t\t\t\
<tt:SimpleItem Name=\"AlarmOffDelay\" Value=\"1000\"></tt:SimpleItem>\n\t\t\t\t\t\t\t\t\t\
<tt:SimpleItem Name=\"ActiveCells\" Value=\"%s\"></tt:SimpleItem>\n\t\t\t\t\t\t\t\t\
</tt:Parameters>\n\t\t\t\t\t\t\t\
</tt:Rule>\n\t\t\t\t\t\t\
</tt:RuleEngineConfiguration>\n\t\t\t\t\t\
</tr2:Analytics>\n\t\t\t\t\t\
\
<tr2:PTZ token=\"PTZConfig_1\">\n\t\t\t\t\
<tt:Name>PTZConfig1</tt:Name>\n\t\t\t\t\
<tt:UseCount>6</tt:UseCount>\n\t\t\t\t\
<tt:NodeToken>PTZNode_0</tt:NodeToken>\n\t\t\t\t\
<tt:DefaultAbsolutePantTiltPositionSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:DefaultAbsolutePantTiltPositionSpace>\
<tt:DefaultAbsoluteZoomPositionSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:DefaultAbsoluteZoomPositionSpace>\
<tt:DefaultRelativePanTiltTranslationSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:DefaultRelativePanTiltTranslationSpace>\
<tt:DefaultRelativeZoomTranslationSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:DefaultRelativeZoomTranslationSpace>\
<tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace>\
<tt:DefaultContinuousZoomVelocitySpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:DefaultContinuousZoomVelocitySpace>\
<tt:DefaultPTZSpeed>\
<tt:PanTilt x=\"1\" y=\"1\" space=\"http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace\"/>\
<tt:Zoom x=\"1.0\" space=\"http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace\"/>\
</tt:DefaultPTZSpeed>\
<tt:DefaultPTZTimeout>PT60S</tt:DefaultPTZTimeout>\
<tt:PanTiltLimits>\
<tt:Range>\
<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:URI>\
<tt:XRange>\
<tt:Min>-1</tt:Min>\
<tt:Max>1</tt:Max>\
</tt:XRange>\
<tt:YRange>\
<tt:Min>-1</tt:Min>\
<tt:Max>1</tt:Max>\
</tt:YRange>\
</tt:Range>\
</tt:PanTiltLimits>\
<tt:ZoomLimits>\
<tt:Range>\
<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:URI>\
<tt:XRange>\
<tt:Min>1.000</tt:Min>\
<tt:Max>%f</tt:Max>\
</tt:XRange>\
</tt:Range>\
</tt:ZoomLimits>\
</tr2:PTZ>\n\t\t\t\t\
<tr2:AudioOutput token=\"AudioOutputConfig_1\">\n\t\t\t\t\t\t\
<tt:Name>AudioOutputConfig1</tt:Name>\n\t\t\t\t\t\t\
<tt:UseCount>6</tt:UseCount>\n\t\t\t\t\t\t\
<tt:OutputToken>AudioOutput_1</tt:OutputToken>\n\t\t\t\t\t\t\
<tt:SendPrimacy>www.onvif.org/ver20/HalfDuplex/Server</tt:SendPrimacy>\n\t\t\t\t\t\t\
<tt:OutputLevel>50</tt:OutputLevel>\n\t\t\t\t\t\
</tr2:AudioOutput>\n\t\t\t\t\t\
\
<tr2:AudioDecoder token=\"AudioDecoderToken_1\">\n\t\t\t\t\t\t\
<tt:Name>AudioDecoder</tt:Name>\n\t\t\t\t\t\t\
<tt:UseCount>6</tt:UseCount>\n\t\t\t\t\t\
</tr2:AudioDecoder>\n\t\t\t\t\
\
</tr2:Configurations>\n\t\t\t\
</tr2:Profiles>\n\t\t\t";

const char *ONVIF_HTTP_PROFILE_1_CONTENT =
    "<trt:Profile%s token=\"%s\" fixed=\"true\">\n\t\t\t\t\
<tt:Name>D01_CH01_%s</tt:Name>\n\t\t\t\t\
\
<tt:VideoSourceConfiguration token=\"VideoSourceConfig_1_1\">\n\t\t\t\t\t\
<tt:Name>VideoSourceConfig1_1</tt:Name>\n\t\t\t\t\t\
<tt:UseCount>6</tt:UseCount>\n\t\t\t\t\t\
<tt:SourceToken>VideoSource_1</tt:SourceToken>\n\t\t\t\t\t\
<tt:Bounds x=\"0\" y=\"0\" width=\"%d\" height=\"%d\"></tt:Bounds>\n\t\t\t\t\
</tt:VideoSourceConfiguration>\n\t\t\t\t\
\
<tt:AudioSourceConfiguration token=\"AudioSourceConfig_1\">\n\t\t\t\t\t\
<tt:Name>AudioSourceConfig1</tt:Name>\n\t\t\t\t\t\
<tt:UseCount>6</tt:UseCount>\n\t\t\t\t\t\
<tt:SourceToken>AudioSource_1</tt:SourceToken>\n\t\t\t\t\
</tt:AudioSourceConfiguration>\n\t\t\t\t\
\
<tt:VideoEncoderConfiguration token=\"VideoEncoderToken_1_1_%d\">\n\t\t\t\t\t\
<tt:Name>%sStream</tt:Name>\n\t\t\t\t\t\
<tt:UseCount>1</tt:UseCount>\n\t\t\t\t\t\
<tt:Encoding>%s</tt:Encoding>\n\t\t\t\t\t\
<tt:Resolution>\n\t\t\t\t\t\t\
<tt:Width>%d</tt:Width>\n\t\t\t\t\t\t\
<tt:Height>%d</tt:Height>\n\t\t\t\t\t\
</tt:Resolution>\n\t\t\t\t\t\
<tt:Quality>%d</tt:Quality>\n\t\t\t\t\t\
<tt:RateControl ConstantBitRate=\"%s\">\n\t\t\t\t\t\t\
<tt:FrameRateLimit>%d</tt:FrameRateLimit>\n\t\t\t\t\t\t\
<tt:EncodingInterval>1</tt:EncodingInterval>\n\t\t\t\t\t\t\
<tt:BitrateLimit>%d</tt:BitrateLimit>\n\t\t\t\t\t\
</tt:RateControl>\n\t\t\t\t\t\
<tt:H264>\n\t\t\t\t\t\t\
<tt:GovLength>%d</tt:GovLength>\n\t\t\t\t\t\t\
<tt:H264Profile>%s</tt:H264Profile>\n\t\t\t\t\t\
</tt:H264>\n\t\t\t\t\t\
<tt:Multicast>\n\t\t\t\t\t\t\
<tt:Address>\n\t\t\t\t\t\t\t\
<tt:Type>IPv4</tt:Type>\n\t\t\t\t\t\t\t\
<tt:IPv4Address>%s</tt:IPv4Address>\n\t\t\t\t\t\t\
</tt:Address>\n\t\t\t\t\t\t\
<tt:Port>%d</tt:Port>\n\t\t\t\t\t\t\
<tt:TTL>255</tt:TTL>\n\t\t\t\t\t\t\
<tt:AutoStart>false</tt:AutoStart>\n\t\t\t\t\t\
</tt:Multicast>\n\t\t\t\t\t\
<tt:SessionTimeout>PT0S</tt:SessionTimeout>\n\t\t\t\t\
</tt:VideoEncoderConfiguration>\n\t\t\t\t\
\
<tt:AudioEncoderConfiguration token=\"AudioEncoderToken_1\">\n\t\t\t\t\t\
<tt:Name>FirstAudio</tt:Name>\n\t\t\t\t\t\
<tt:UseCount>3</tt:UseCount>\n\t\t\t\t\t\
<tt:Encoding>G711</tt:Encoding>\n\t\t\t\t\t\
<tt:Bitrate>64</tt:Bitrate>\n\t\t\t\t\t\
<tt:SampleRate>8</tt:SampleRate>\n\t\t\t\t\t\
<tt:Multicast>\n\t\t\t\t\t\t\
<tt:Address>\n\t\t\t\t\t\t\t\
<tt:Type>IPv4</tt:Type>\n\t\t\t\t\t\t\t\
<tt:IPv4Address>%s</tt:IPv4Address>\n\t\t\t\t\t\t\
</tt:Address>\n\t\t\t\t\t\t\
<tt:Port>%d</tt:Port>\n\t\t\t\t\t\t\
<tt:TTL>255</tt:TTL>\n\t\t\t\t\t\t\
<tt:AutoStart>false</tt:AutoStart>\n\t\t\t\t\t\
</tt:Multicast>\n\t\t\t\t\t\
<tt:SessionTimeout>PT0S</tt:SessionTimeout>\n\t\t\t\t\
</tt:AudioEncoderConfiguration>\n\t\t\t\t\
\
<tt:VideoAnalyticsConfiguration token=\"VideoAnalytics_1\">\n\t\t\t\t\t\
<tt:Name>VideoAnalytics1</tt:Name>\n\t\t\t\t\t\
<tt:UseCount>6</tt:UseCount>\n\t\t\t\t\t\
<tt:AnalyticsEngineConfiguration>\n\t\t\t\t\t\t\
<tt:AnalyticsModule Type=\"tt:CellMotionEngine\" Name=\"MyCellMotionModule\">\n\t\t\t\t\t\t\t\
<tt:Parameters>\n\t\t\t\t\t\t\t\t\
<tt:SimpleItem Name=\"Sensitivity\" Value=\"%d\"></tt:SimpleItem>\n\t\t\t\t\t\t\t\t\
<tt:ElementItem Name=\"Layout\"><tt:CellLayout Columns=\"22\" Rows=\"18\"><tt:Transformation><tt:Translate x=\"-1.000000\" y=\"-1.000000\"/><tt:Scale x=\"0.045455\" y=\"-0.055556\"/></tt:Transformation></tt:CellLayout></tt:ElementItem>\n\t\t\t\t\t\t\t\
</tt:Parameters>\n\t\t\t\t\t\t\
</tt:AnalyticsModule>\n\t\t\t\t\t\
</tt:AnalyticsEngineConfiguration>\n\t\t\t\t\t\
<tt:RuleEngineConfiguration>\n\t\t\t\t\t\t\
<tt:Rule Type=\"tt:CellMotionDetector\" Name=\"MyCellMotionDetectorRule\">\n\t\t\t\t\t\t\t\
<tt:Parameters>\n\t\t\t\t\t\t\t\t\
<tt:SimpleItem Name=\"MinCount\" Value=\"5\"></tt:SimpleItem>\n\t\t\t\t\t\t\t\t\
<tt:SimpleItem Name=\"AlarmOnDelay\" Value=\"1000\"></tt:SimpleItem>\n\t\t\t\t\t\t\t\t\
<tt:SimpleItem Name=\"AlarmOffDelay\" Value=\"1000\"></tt:SimpleItem>\n\t\t\t\t\t\t\t\t\
<tt:SimpleItem Name=\"ActiveCells\" Value=\"%s\"></tt:SimpleItem>\n\t\t\t\t\t\t\t\
</tt:Parameters>\n\t\t\t\t\t\t\
</tt:Rule>\n\t\t\t\t\t\
</tt:RuleEngineConfiguration>\n\t\t\t\t\
</tt:VideoAnalyticsConfiguration>\n\t\t\t\t\
\
<tt:PTZConfiguration token=\"PTZConfig_1\">\
<tt:Name>PTZConfig1</tt:Name>\
<tt:UseCount>6</tt:UseCount>\
<tt:NodeToken>PTZNode_0</tt:NodeToken>\
<tt:DefaultAbsolutePantTiltPositionSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:DefaultAbsolutePantTiltPositionSpace>\
<tt:DefaultAbsoluteZoomPositionSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:DefaultAbsoluteZoomPositionSpace>\
<tt:DefaultRelativePanTiltTranslationSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:DefaultRelativePanTiltTranslationSpace>\
<tt:DefaultRelativeZoomTranslationSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:DefaultRelativeZoomTranslationSpace>\
<tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace>\
<tt:DefaultContinuousZoomVelocitySpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:DefaultContinuousZoomVelocitySpace>\
<tt:DefaultPTZSpeed>\
<tt:PanTilt x=\"1\" y=\"1\" space=\"http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace\"/>\
<tt:Zoom x=\"1.0\" space=\"http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace\"/>\
</tt:DefaultPTZSpeed>\
<tt:DefaultPTZTimeout>PT60S</tt:DefaultPTZTimeout>\
<tt:PanTiltLimits>\
<tt:Range>\
<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:URI>\
<tt:XRange>\
<tt:Min>-1</tt:Min>\
<tt:Max>1</tt:Max>\
</tt:XRange>\
<tt:YRange>\
<tt:Min>-1</tt:Min>\
<tt:Max>1</tt:Max>\
</tt:YRange>\
</tt:Range>\
</tt:PanTiltLimits>\
<tt:ZoomLimits>\
<tt:Range>\
<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:URI>\
<tt:XRange>\
<tt:Min>1.000</tt:Min>\
<tt:Max>%f</tt:Max>\
</tt:XRange>\
</tt:Range>\
</tt:ZoomLimits>\
</tt:PTZConfiguration>\
\
<tt:Extension>\n\t\t\t\t\t\
<tt:AudioOutputConfiguration token=\"AudioOutputConfig_1\">\n\t\t\t\t\t\t\
<tt:Name>AudioOutputConfig1</tt:Name>\n\t\t\t\t\t\t\
<tt:UseCount>6</tt:UseCount>\n\t\t\t\t\t\t\
<tt:OutputToken>AudioOutput_1</tt:OutputToken>\n\t\t\t\t\t\t\
<tt:SendPrimacy>www.onvif.org/ver20/HalfDuplex/Server</tt:SendPrimacy>\n\t\t\t\t\t\t\
<tt:OutputLevel>50</tt:OutputLevel>\n\t\t\t\t\t\
</tt:AudioOutputConfiguration>\n\t\t\t\t\t\
<tt:AudioDecoderConfiguration token=\"AudioDecoderToken_1\">\n\t\t\t\t\t\t\
<tt:Name>AudioDecoder</tt:Name>\n\t\t\t\t\t\t\
<tt:UseCount>6</tt:UseCount>\n\t\t\t\t\t\
</tt:AudioDecoderConfiguration>\n\t\t\t\t\
</tt:Extension>\n\t\t\t\
</trt:Profile%s>\n\t\t\t";

/*<tt:MetadataConfiguration token=\"MetaDataToken\">\n\t\t\t\t\t\
<tt:Name>MetaData</tt:Name>\n\t\t\t\t\t\
<tt:UseCount>3</tt:UseCount>\n\t\t\t\t\t\
<tt:PTZStatus>\n\t\t\t\t\t\t\
<tt:Status>false</tt:Status>\n\t\t\t\t\t\t\
<tt:Position>false</tt:Position>\n\t\t\t\t\t\
</tt:PTZStatus>\n\t\t\t\t\t\
<tt:Analytics>false</tt:Analytics>\n\t\t\t\t\t\
<tt:Multicast>\n\t\t\t\t\t\t\
<tt:Address>\n\t\t\t\t\t\t\t\
<tt:Type>IPv4</tt:Type>\n\t\t\t\t\t\t\t\
<tt:IPv4Address>0.0.0.0</tt:IPv4Address>\n\t\t\t\t\t\t\
</tt:Address>\n\t\t\t\t\t\t\
<tt:Port>8864</tt:Port>\n\t\t\t\t\t\t\
<tt:TTL>128</tt:TTL>\n\t\t\t\t\t\t\
<tt:AutoStart>false</tt:AutoStart>\n\t\t\t\t\t\
</tt:Multicast>\n\t\t\t\t\t\
<tt:SessionTimeout>PT00H00M05S</tt:SessionTimeout>\n\t\t\t\t\
</tt:MetadataConfiguration>\n\t\t\t\t\*/

const char *ONVIF_HTTP_PROFILE_1_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<trt:GetProfileResponse>\n\t\t\t\
%s\
</trt:GetProfileResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_PROFILE_2_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tr2:GetProfile%sResponse>\n\t\t\t\
%s\
</tr2:GetProfile%sResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_PROFILES_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<trt:GetProfilesResponse>\n\t\t\t\
%s\
</trt:GetProfilesResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";




const char *ONVIF_HTTP_GET_SERVICE_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tds:GetServicesResponse>\n\t\t\t\
<tds:Service>\n\t\t\t\t\
<tds:Namespace>http://www.onvif.org/ver10/device/wsdl</tds:Namespace>\n\t\t\t\t\
<tds:XAddr>%s/onvif/device_service</tds:XAddr>\n\t\t\t\t\
<tds:Version>\n\t\t\t\t\t\
<tt:Major>17</tt:Major>\n\t\t\t\t\t\
<tt:Minor>12</tt:Minor>\n\t\t\t\t\
</tds:Version>\n\t\t\t\
</tds:Service>\n\t\t\t\
<tds:Service>\n\t\t\t\t\
<tds:Namespace>http://www.onvif.org/ver10/media/wsdl</tds:Namespace>\n\t\t\t\t\
<tds:XAddr>%s/onvif/Media</tds:XAddr>\n\t\t\t\t\
<tds:Version>\n\t\t\t\t\t\
<tt:Major>17</tt:Major>\n\t\t\t\t\t\
<tt:Minor>12</tt:Minor>\n\t\t\t\t\
</tds:Version>\n\t\t\t\
</tds:Service>\n\t\t\t\
<tds:Service>\n\t\t\t\t\
<tds:Namespace>http://www.onvif.org/ver20/media/wsdl</tds:Namespace>\n\t\t\t\t\
<tds:XAddr>%s/onvif/Media20</tds:XAddr>\n\t\t\t\t\
<tds:Version>\n\t\t\t\t\t\
<tt:Major>17</tt:Major>\n\t\t\t\t\t\
<tt:Minor>12</tt:Minor>\n\t\t\t\t\
</tds:Version>\n\t\t\t\
</tds:Service>\n\t\t\t\
<tds:Service>\n\t\t\t\t\
<tds:Namespace>http://www.onvif.org/ver20/imaging/wsdl</tds:Namespace>\n\t\t\t\t\
<tds:XAddr>%s/onvif/Imaging</tds:XAddr>\n\t\t\t\t\
<tds:Version>\n\t\t\t\t\t\
<tt:Major>17</tt:Major>\n\t\t\t\t\t\
<tt:Minor>12</tt:Minor>\n\t\t\t\t\
</tds:Version>\n\t\t\t\
</tds:Service>\n\t\t\t\
<tds:Service>\n\t\t\t\t\
<tds:Namespace>http://www.onvif.org/ver10/events/wsdl</tds:Namespace>\n\t\t\t\t\
<tds:XAddr>%s/onvif/Events</tds:XAddr>\n\t\t\t\t\
<tds:Version>\n\t\t\t\t\t\
<tt:Major>17</tt:Major>\n\t\t\t\t\t\
<tt:Minor>12</tt:Minor>\n\t\t\t\t\
</tds:Version>\n\t\t\t\
</tds:Service>\n\t\t\t\
<tds:Service>\n\t\t\t\t\
<tds:Namespace>http://www.onvif.org/ver20/analytics/wsdl</tds:Namespace>\n\t\t\t\t\
<tds:XAddr>%s/onvif/Analytics</tds:XAddr>\n\t\t\t\t\
<tds:Version>\n\t\t\t\t\t\
<tt:Major>17</tt:Major>\n\t\t\t\t\t\
<tt:Minor>12</tt:Minor>\n\t\t\t\t\
</tds:Version>\n\t\t\t\
</tds:Service>\n\t\t\t\t\
<tds:Service>\n\t\t\t\t\
<tds:Namespace>http://www.onvif.org/ver20/ptz/wsdl</tds:Namespace>\n\t\t\t\t\
<tds:XAddr>%s/onvif/ptz_service</tds:XAddr>\n\t\t\t\t\
<tds:Version>\n\t\t\t\t\t\
<tt:Major>17</tt:Major>\n\t\t\t\t\t\
<tt:Minor>12</tt:Minor>\n\t\t\t\t\
</tds:Version>\n\t\t\t\
</tds:Service>\n\t\t\
</tds:GetServicesResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GET_SERVICE_WITH_CAP_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tds:GetServicesResponse>\n\t\t\t\
<tds:Service>\n\t\t\t\t\
<tds:Namespace>http://www.onvif.org/ver10/device/wsdl</tds:Namespace>\n\t\t\t\t\
<tds:XAddr>%s/onvif/device_service</tds:XAddr>\n\t\t\t\t\
<tds:Capabilities>\n\t\t\t\t\t\
<tds:Capabilities>\n\t\t\t\t\t\t\
<tds:Network IPFilter=\"false\" ZeroConfiguration=\"false\" IPVersion6=\"false\" DynDNS=\"false\" Dot11Configuration=\"false\" Dot1XConfigurations=\"0\" HostnameFromDHCP=\"true\" NTP=\"1\" DHCPv6=\"false\"></tds:Network>\n\t\t\t\t\t\t\
<tds:Security TLS1.0=\"false\" TLS1.1=\"true\" TLS1.2=\"true\" OnboardKeyGeneration=\"false\" AccessPolicyConfig=\"false\" DefaultAccessPolicy=\"true\" Dot1X=\"false\" RemoteUserHandling=\"false\" X.509Token=\"false\" SAMLToken=\"false\" KerberosToken=\"false\" UsernameToken=\"false\" HttpDigest=\"false\" RELToken=\"false\" SupportedEAPMethods=\"0\" MaxUsers=\"10\" MaxUserNameLength=\"32\" MaxPasswordLength=\"16\"></tds:Security>\n\t\t\t\t\t\t\
<tds:System DiscoveryResolve=\"false\" DiscoveryBye=\"true\" RemoteDiscovery=\"false\" SystemBackup=\"false\" SystemLogging=\"true\" FirmwareUpgrade=\"true\" HttpFirmwareUpgrade=\"true\" HttpSystemBackup=\"false\" HttpSystemLogging=\"false\" HttpSupportInformation=\"false\" StorageConfiguration=\"true\" MaxStorageConfigurations=\"8\"></tds:System>\n\t\t\t\t\t\t\
</tds:Capabilities>\n\t\t\t\t\t\
</tds:Capabilities>\n\t\t\t\t\
<tds:Version>\n\t\t\t\t\t\
<tt:Major>17</tt:Major>\n\t\t\t\t\t\
<tt:Minor>12</tt:Minor>\n\t\t\t\t\
</tds:Version>\n\t\t\t\
</tds:Service>\n\t\t\t\
<tds:Service>\n\t\t\t\t\
<tds:Namespace>http://www.onvif.org/ver10/media/wsdl</tds:Namespace>\n\t\t\t\t\
<tds:XAddr>%s/onvif/Media</tds:XAddr>\n\t\t\t\t\
<tds:Capabilities><trt:Capabilities SnapshotUri=\"true\" Rotation=\"false\" VideoSourceMode=\"false\" OSD=\"true\" EXICompression=\"false\"><trt:ProfileCapabilities MaximumNumberOfProfiles=\"2\"></trt:ProfileCapabilities><trt:StreamingCapabilities RTPMulticast=\"false\" RTP_TCP=\"false\" RTP_RTSP_TCP=\"true\" NonAggregateControl=\"true\"></trt:StreamingCapabilities></trt:Capabilities></tds:Capabilities>\
<tds:Version>\n\t\t\t\t\t\
<tt:Major>17</tt:Major>\n\t\t\t\t\t\
<tt:Minor>12</tt:Minor>\n\t\t\t\t\
</tds:Version>\n\t\t\t\
</tds:Service>\n\t\t\t\
<tds:Service>\n\t\t\t\t\
<tds:Namespace>http://www.onvif.org/ver20/imaging/wsdl</tds:Namespace>\n\t\t\t\t\
<tds:XAddr>%s/onvif/Imaging</tds:XAddr>\n\t\t\t\t\
<tds:Capabilities>\n\t\t\t\t\
<timg:Capabilities ImageStabilization=\"false\" Presets=\"false\"></timg:Capabilities>\n\t\t\t\t\
</tds:Capabilities>\n\t\t\t\t\
<tds:Version>\n\t\t\t\t\t\
<tt:Major>17</tt:Major>\n\t\t\t\t\t\
<tt:Minor>12</tt:Minor>\n\t\t\t\t\
</tds:Version>\n\t\t\t\
</tds:Service>\n\t\t\t\
<tds:Service>\
<tds:Namespace>http://www.onvif.org/ver10/events/wsdl</tds:Namespace>\n\t\t\t\t\
<tds:XAddr>%s/onvif/Events</tds:XAddr>\n\t\t\t\t\
<tds:Capabilities>\n\t\t\t\t\t\
<tev:Capabilities WSSubscriptionPolicySupport=\"true\" WSPullPointSupport=\"true\" WSPausableSubscriptionManagerInterfaceSupport=\"false\" MaxNotificationProducers=\"5\" MaxPullPoints=\"5\" PersistentNotificationStorage=\"false\">\n\t\t\t\t\
</tev:Capabilities>\n\t\t\t\t\t\
</tds:Capabilities>\n\t\t\t\t\
<tds:Version>\n\t\t\t\t\t\
<tt:Major>17</tt:Major>\n\t\t\t\t\t\
<tt:Minor>12</tt:Minor>\n\t\t\t\t\
</tds:Version>\n\t\t\t\
</tds:Service>\n\t\t\t\
<tds:Service>\n\t\t\t\t\
<tds:Namespace>http://www.onvif.org/ver20/analytics/wsdl</tds:Namespace>\n\t\t\t\t\
<tds:XAddr>%s/onvif/Analytics</tds:XAddr>\n\t\t\t\t\
<tds:Capabilities>\
<tan:Capabilities RuleSupport=\"true\" AnalyticsModuleSupport=\"true\" CellBasedSceneDescriptionSupported=\"true\"></tan:Capabilities>\
</tds:Capabilities>\n\t\t\t\t\
<tds:Version>\n\t\t\t\t\t\
<tt:Major>17</tt:Major>\n\t\t\t\t\t\
<tt:Minor>12</tt:Minor>\n\t\t\t\t\
</tds:Version>\n\t\t\t\t\
</tds:Service>\n\t\t\t\
<tds:Service>\n\t\t\t\t\
<tds:Namespace>http://www.onvif.org/ver20/media/wsdl</tds:Namespace>\n\t\t\t\t\
<tds:XAddr>%s/onvif/Media20</tds:XAddr>\n\t\t\t\t\
<tds:Capabilities>\n\
<tr2:Capabilities SnapshotUri=\"true\" Rotation=\"false\" VideoSourceMode=\"false\" OSD=\"true\" Mask=\"true\">\n\
<tr2:ProfileCapabilities MaximumNumberOfProfiles=\"10\" ConfigurationsSupported=\"VideoSource AudioSource VideoEncoder AudioEncoder AudioOutput AudioDecoder PTZ\"></tr2:ProfileCapabilities>\n\
<tr2:StreamingCapabilities RTSPStreaming=\"true\" RTPMulticast=\"false\" RTP_RTSP_TCP=\"true\" NonAggregateControl=\"false\"></tr2:StreamingCapabilities>\n\
</tr2:Capabilities>\n\
</tds:Capabilities>\n\
<tds:Version>\n\t\t\t\t\t\
<tt:Major>17</tt:Major>\n\t\t\t\t\t\
<tt:Minor>12</tt:Minor>\n\t\t\t\t\
</tds:Version>\n\t\t\t\
</tds:Service>\n\t\t\t\
<tds:Service>\n\t\t\t\t\
<tds:Namespace>http://www.onvif.org/ver20/ptz/wsdl</tds:Namespace>\n\t\t\t\t\
<tds:XAddr>%s/onvif/ptz_service</tds:XAddr>\n\t\t\t\t\
<tds:Capabilities>\n\t\t\t\t\t\
<tptz:Capabilities EFlip=\"false\" Reverse=\"false\" GetCompatibleConfigurations=\"false\" MoveStatus=\"true\" StatusPosition=\"true\" />\n\t\t\t\t\
</tds:Capabilities>\n\t\t\t\t\
<tds:Version>\n\t\t\t\t\t\
<tt:Major>17</tt:Major>\n\t\t\t\t\t\
<tt:Minor>12</tt:Minor>\n\t\t\t\t\
</tds:Version>\n\t\t\t\
</tds:Service>\n\t\t\
</tds:GetServicesResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GET_VIDEO_SOURCE_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:GetVideoSourcesResponse>\n\t\t\t\
<%s:VideoSources token=\"VideoSource_1\">\n\t\t\t\t\
<tt:Framerate>%d</tt:Framerate>\n\t\t\t\t\
<tt:Resolution>\n\t\t\t\t\t\
<tt:Width>%d</tt:Width>\n\t\t\t\t\t\
<tt:Height>%d</tt:Height>\n\t\t\t\t\
</tt:Resolution>\n\t\t\t\t\
</%s:VideoSources>\n\t\t\
</%s:GetVideoSourcesResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";


const char *ONVIF_HTTP_GET_STREAM_URI_1_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<trt:GetStreamUriResponse>\n\t\t\t\
<trt:MediaUri>\n\t\t\t\t\
<tt:Uri>%s</tt:Uri>\n\t\t\t\t\
<tt:InvalidAfterConnect>false</tt:InvalidAfterConnect>\n\t\t\t\t\
<tt:InvalidAfterReboot>false</tt:InvalidAfterReboot>\n\t\t\t\t\
<tt:Timeout>PT00H00M05S</tt:Timeout>\n\t\t\t\
</trt:MediaUri>\n\t\t\
</trt:GetStreamUriResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GET_STREAM_URI_2_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tr2:GetStreamUriResponse>\n\t\t\t\
<tr2:Uri>%s</tr2:Uri>\n\t\t\
</tr2:GetStreamUriResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";


const char *ONVIF_HTTP_GET_VIDEO_SOURCE_CFG_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<trt:GetVideoSourceConfigurationResponse>\n\t\t\t\
<trt:Configuration token=\"VideoSourceConfig_1_1\">\n\t\t\t\t\
<tt:Name>VideoSourceConfig1_1</tt:Name>\n\t\t\t\t\
<tt:UseCount>6</tt:UseCount>\n\t\t\t\t\
<tt:SourceToken>VideoSource_1</tt:SourceToken>\n\t\t\t\t\
<tt:Bounds x=\"0\" y=\"0\" width=\"%d\" height=\"%d\"></tt:Bounds>\n\t\t\t\
</trt:Configuration>\n\t\t\
</trt:GetVideoSourceConfigurationResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GET_OSD_TOKEN_1 =
    "<%s:OSD%s token=\"OSDToken_%s\">\n\t\t\t\t\
<tt:VideoSourceConfigurationToken>VideoSourceConfig_1_1</tt:VideoSourceConfigurationToken>\n\t\t\t\t\
<tt:Type>Text</tt:Type>\n\t\t\t\t\
<tt:Position>\n\t\t\t\t\t\
<tt:Type>Custom</tt:Type>\n\t\t\t\t\t\
<tt:Pos x=\"%f\" y=\"%f\"></tt:Pos>\n\t\t\t\t\
</tt:Position>\n\t\t\t\t\
<tt:TextString>\n\t\t\t\t\t\
<tt:Type>Plain</tt:Type>\n\t\t\t\t\t\
<tt:FontSize>%d</tt:FontSize>\n\t\t\t\t\t\
<tt:PlainText>%s</tt:PlainText>\n\t\t\t\t\t\
<tt:Extension><tt:ChannelName>true</tt:ChannelName></tt:Extension>\n\t\t\t\t\
</tt:TextString>\n\t\t\t\
</%s:OSD%s>\n\t\t\t";

const char *ONVIF_HTTP_GET_OSD_TOKEN_2 =
    "<%s:OSD%s token=\"OSDToken_%s\">\n\t\t\t\t\
<tt:VideoSourceConfigurationToken>VideoSourceConfig_1_1</tt:VideoSourceConfigurationToken>\n\t\t\t\t\
<tt:Type>Text</tt:Type>\n\t\t\t\t\
<tt:Position>\n\t\t\t\t\t\
<tt:Type>Custom</tt:Type>\n\t\t\t\t\t\
<tt:Pos x=\"%f\" y=\"%f\"></tt:Pos>\n\t\t\t\t\
</tt:Position>\n\t\t\t\t\
<tt:TextString>\n\t\t\t\t\t\
<tt:Type>DateAndTime</tt:Type>\n\t\t\t\t\t\
<tt:DateFormat>%s</tt:DateFormat>\n\t\t\t\t\t\
<tt:TimeFormat>%s</tt:TimeFormat>\n\t\t\t\t\t\
<tt:FontSize>%d</tt:FontSize>\n\t\t\t\t\t\
<tt:Extension><tt:ChannelName>false</tt:ChannelName></tt:Extension>\n\t\t\t\t\
</tt:TextString>\n\t\t\t\
</%s:OSD%s>\n\t\t";

#if (defined ONVIF_EXT_CUSTOM_OSD)
const char *ONVIF_HTTP_GET_OSD_2_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:GetOSDsResponse>\n\t\t\
%s%s%s%s%s%s\
</%s:GetOSDsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";
#else
const char *ONVIF_HTTP_GET_OSD_2_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:GetOSDsResponse>\n\t\t\
%s%s%s\
</%s:GetOSDsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";
#endif

const char *ONVIF_HTTP_SET_OSD_2_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:SetOSDResponse>\n\t\t\
</%s:SetOSDResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_SET_OSD_2_BODY_ver20 =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tr2:SetOSDResponse/>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";



const char *ONVIF_HTTP_IMAGE_SET_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<timg:GetImagingSettingsResponse>\n\t\t\t\
<timg:ImagingSettings>\n\t\t\t\t\
<tt:Brightness>%d</tt:Brightness>\n\t\t\t\t\
<tt:ColorSaturation>%d</tt:ColorSaturation>\n\t\t\t\t\
<tt:Contrast>%d</tt:Contrast>\n\t\t\t\t\
<tt:Sharpness>%d</tt:Sharpness>\n\t\t\t\
</timg:ImagingSettings>\n\t\t\
</timg:GetImagingSettingsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GET_OPTIONS_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<timg:GetOptionsResponse>\n\t\t\t\
<timg:ImagingOptions>\n\t\t\t\t\
<tt:Brightness>\n\t\t\t\t\t\
<tt:Min>0</tt:Min>\n\t\t\t\t\t\
<tt:Max>255</tt:Max>\n\t\t\t\t\
</tt:Brightness>\n\t\t\t\t\
<tt:ColorSaturation>\n\t\t\t\t\t\
<tt:Min>0</tt:Min>\n\t\t\t\t\t\
<tt:Max>255</tt:Max>\n\t\t\t\t\
</tt:ColorSaturation>\n\t\t\t\t\
<tt:Contrast>\n\t\t\t\t\t\
<tt:Min>0</tt:Min>\n\t\t\t\t\t\
<tt:Max>255</tt:Max>\n\t\t\t\t\
</tt:Contrast>\n\t\t\t\t\
<tt:Sharpness>\n\t\t\t\t\t\
<tt:Min>0</tt:Min>\n\t\t\t\t\t\
<tt:Max>255</tt:Max>\n\t\t\t\t\
</tt:Sharpness>\n\t\t\t\
</timg:ImagingOptions>\n\t\t\
</timg:GetOptionsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_SET_IMAGING_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<timg:SetImagingSettingsResponse></timg:SetImagingSettingsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ResolutionsStr =
    "<tt:ResolutionsAvailable>\n\t\t\t\t\t\t\
<tt:Width>%d</tt:Width>\n\t\t\t\t\t\t\
<tt:Height>%d</tt:Height>\n\t\t\t\t\t\
</tt:ResolutionsAvailable>\n\t\t\t\t\t";

const char *ONVIF_HTTP_GetVideoEncoderConfigurationOptions_H264 =
    "<tt:H264>\n\t\t\t\t\t\
%s\
<tt:GovLengthRange>\n\t\t\t\t\t\t\
<tt:Min>%d</tt:Min>\n\t\t\t\t\t\t\
<tt:Max>%d</tt:Max>\n\t\t\t\t\t\
</tt:GovLengthRange>\n\t\t\t\t\t\
<tt:FrameRateRange>\n\t\t\t\t\t\t\
<tt:Min>%d</tt:Min>\n\t\t\t\t\t\t\
<tt:Max>%d</tt:Max>\n\t\t\t\t\t\
</tt:FrameRateRange>\n\t\t\t\t\t\
<tt:EncodingIntervalRange>\n\t\t\t\t\t\t\
<tt:Min>1</tt:Min>\n\t\t\t\t\t\t\
<tt:Max>1</tt:Max>\n\t\t\t\t\t\
</tt:EncodingIntervalRange>\n\t\t\t\t\t\
<tt:H264ProfilesSupported>Baseline</tt:H264ProfilesSupported>\n\t\t\t\t\t\
<tt:H264ProfilesSupported>Main</tt:H264ProfilesSupported>\n\t\t\t\t\t\
<tt:H264ProfilesSupported>High</tt:H264ProfilesSupported>\n\t\t\t\t\t\
<tt:BitrateRange>\n\t\t\t\t\t\t\
<tt:Min>%d</tt:Min>\n\t\t\t\t\t\t\
<tt:Max>%d</tt:Max>\n\t\t\t\t\t\
</tt:BitrateRange>\n\t\t\t\t\
</tt:H264>\n\t\t\t\t";

const char *ONVIF_HTTP_GetVideoEncoderConfigurationOptions_MJPEG =
    "<tt:JPEG>\n\t\t\t\t\t\
%s\
<tt:FrameRateRange>\n\t\t\t\t\t\t\
<tt:Min>%d</tt:Min>\n\t\t\t\t\t\t\
<tt:Max>%d</tt:Max>\n\t\t\t\t\t\
</tt:FrameRateRange>\n\t\t\t\t\t\
<tt:EncodingIntervalRange>\n\t\t\t\t\t\t\
<tt:Min>1</tt:Min>\n\t\t\t\t\t\t\
<tt:Max>1</tt:Max>\n\t\t\t\t\t\
</tt:EncodingIntervalRange>\n\t\t\t\t\t\
<tt:BitrateRange>\n\t\t\t\t\t\t\
<tt:Min>%d</tt:Min>\n\t\t\t\t\t\t\
<tt:Max>%d</tt:Max>\n\t\t\t\t\t\
</tt:BitrateRange>\n\t\t\t\t\t\
</tt:JPEG>\n\t\t\t";

const char *ONVIF_HTTP_GetVideoEncoderConfigurationOptions_Extension =
    "<tt:Extension>\n\t\t\t\t\
%s\
</tt:Extension>\n\t\t\t";

const char *ONVIF_HTTP_GetVideoEncoderConfigurationOptions =
    "<trt:Options>\n\t\t\t\t\
<tt:QualityRange>\n\t\t\t\t\t\
<tt:Min>%d</tt:Min>\n\t\t\t\t\t\
<tt:Max>%d</tt:Max>\n\t\t\t\t\
</tt:QualityRange>\n\t\t\t\t\
%s\
%s\
</trt:Options>\n\t\t";


const char *ONVIF_HTTP_GetVideoEncoderConfigurationOptions_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<trt:GetVideoEncoderConfigurationOptionsResponse>\n\t\t\t\
%s\
</trt:GetVideoEncoderConfigurationOptionsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ResolutionsStr2 =
    "<tt:ResolutionsAvailable>\n\t\t\t\t\t\
<tt:Width>%d</tt:Width>\n\t\t\t\t\t\
<tt:Height>%d</tt:Height>\n\t\t\t\t\
</tt:ResolutionsAvailable>\n\t\t\t\t";

const char *ONVIF_HTTP_GetVideoEncoderConfigurationOptions_2 =
    "<tr2:Options GovLengthRange=\"%d %d\" FrameRatesSupported=\"%s\" ConstantBitRateSupported=\"true\">\n\t\t\t\t\
<tt:Encoding>%s</tt:Encoding>\n\t\t\t\t\
<tt:QualityRange>\n\t\t\t\t\t\
<tt:Min>%d</tt:Min>\n\t\t\t\t\t\
<tt:Max>%d</tt:Max>\n\t\t\t\t\
</tt:QualityRange>\n\t\t\t\t\
%s\
<tt:BitrateRange>\n\t\t\t\t\t\
<tt:Min>%d</tt:Min>\n\t\t\t\t\t\
<tt:Max>%d</tt:Max>\n\t\t\t\t\
</tt:BitrateRange>\n\t\t\
</tr2:Options>\n\t\t\t";


const char *ONVIF_HTTP_GetVideoEncoderConfigurationOptions_2_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tr2:GetVideoEncoderConfigurationOptionsResponse>\n\t\t\t\
%s\
</tr2:GetVideoEncoderConfigurationOptionsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";


const char *ONVIF_HTTP_SetVideoEncoderConfiguration_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:SetVideoEncoderConfigurationResponse></%s:SetVideoEncoderConfigurationResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_SetNetworkInterfaces_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tds:SetNetworkInterfacesResponse>\n\t\t\t\
<tds:RebootNeeded>false</tds:RebootNeeded>\n\t\t\
</tds:SetNetworkInterfacesResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_SetNetworkDefaultGateway_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tds:SetNetworkDefaultGatewayResponse>\n\t\t\
</tds:SetNetworkDefaultGatewayResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";


const char *ONVIF_HTTP_SetVideoAnalyticsConfigurations_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<trt:SetVideoAnalyticsConfigurationResponse></trt:SetVideoAnalyticsConfigurationResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_ModifyAnalyticsModules_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tan:ModifyAnalyticsModulesResponse></tan:ModifyAnalyticsModulesResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_ModifyRules_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tan:ModifyRulesResponse></tan:ModifyRulesResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GET_OSDOPTIONS =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:GetOSDOptionsResponse>\n\t\t\t\
<%s:OSDOptions>\n\t\t\t\t\
<tt:MaximumNumberOfOSDs Total=\"3\" PlainText=\"2\" DateAndTime=\"1\"></tt:MaximumNumberOfOSDs>\n\t\t\t\t\
<tt:Type>Text</tt:Type>\n\t\t\t\t\
<tt:PositionOption>Custom</tt:PositionOption>\n\t\t\t\t\
<tt:TextOption>\n\t\t\t\t\t\
<tt:Type>Plain</tt:Type>\n\t\t\t\t\t\
<tt:Type>Date</tt:Type>\n\t\t\t\t\t\
<tt:Type>Time</tt:Type>\n\t\t\t\t\t\
<tt:Type>DateAndTime</tt:Type>\n\t\t\t\t\t\
<tt:FontSizeRange>\n\t\t\t\t\t\t\
<tt:Min>1</tt:Min>\n\t\t\t\t\t\t\
<tt:Max>5</tt:Max>\n\t\t\t\t\t\
</tt:FontSizeRange>\n\t\t\t\t\t\
<tt:DateFormat>yyyy-MM-dd</tt:DateFormat>\n\t\t\t\t\t\
<tt:DateFormat>MM-dd-yyyy</tt:DateFormat>\n\t\t\t\t\t\
<tt:DateFormat>dd-MM-yyyy</tt:DateFormat>\n\t\t\t\t\t\
<tt:TimeFormat>HH:mm:ss</tt:TimeFormat>\n\t\t\t\t\t\
<tt:TimeFormat>hh:mm:ss tt</tt:TimeFormat>\n\t\t\t\t\
</tt:TextOption>\n\t\t\t\
</%s:OSDOptions>\n\t\t\
</%s:GetOSDOptionsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GET_VIDEO_ENCODER_CONFIGURATION  =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:GetVideoEncoderConfigurationResponse>\n\t\t\t\
<%s:Configuration token=\"VideoEncoder%sToken_%d_%d_%d\">\n\t\t\t\t\
<tt:Name>%s</tt:Name>\n\t\t\t\t\
<tt:UseCount>1</tt:UseCount>\n\t\t\t\t\
<tt:Encoding>%s</tt:Encoding>\n\t\t\t\t\
<tt:Resolution>\n\t\t\t\t\t\
<tt:Width>%d</tt:Width>\n\t\t\t\t\t\
<tt:Height>%d</tt:Height>\n\t\t\t\t\
</tt:Resolution>\n\t\t\t\t\
<tt:Quality>%d</tt:Quality>\n\t\t\t\t\
<tt:RateControl>\n\t\t\t\t\t\
<tt:FrameRateLimit>%d</tt:FrameRateLimit>\n\t\t\t\t\t\
<tt:EncodingInterval>1</tt:EncodingInterval>\n\t\t\t\t\t\
<tt:BitrateLimit>%d</tt:BitrateLimit>\n\t\t\t\t\
</tt:RateControl>\n\t\t\t\t\
<tt:H264>\n\t\t\t\t\t\
<tt:GovLength>%d</tt:GovLength>\n\t\t\t\t\t\
<tt:H264Profile>%s</tt:H264Profile>\n\t\t\t\t\
</tt:H264>\n\t\t\t\t\
<tt:Multicast>\n\t\t\t\t\t\
<tt:Address>\n\t\t\t\t\t\t\
<tt:Type>IPv4</tt:Type>\n\t\t\t\t\t\t\
<tt:IPv4Address>%s</tt:IPv4Address>\n\t\t\t\t\t\
</tt:Address>\n\t\t\t\t\t\
<tt:Port>%d</tt:Port>\n\t\t\t\t\t\
<tt:TTL>255</tt:TTL>\n\t\t\t\t\t\
<tt:AutoStart>true</tt:AutoStart>\n\t\t\t\t\
</tt:Multicast>\n\t\t\t\t\
<tt:SessionTimeout>PT0S</tt:SessionTimeout>\n\t\t\t\
</%s:Configuration>\n\t\t\
</%s:GetVideoEncoderConfigurationResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GET_VIDEO_ENCODER_CONFIGURATIONS_ITEM  =
    "<%s:Configurations token=\"VideoEncoder%sToken_%d_%d_%d\" GovLength=\"%d\">\n\t\t\t\t\
<tt:Name>%s</tt:Name>\n\t\t\t\t\
<tt:UseCount>1</tt:UseCount>\n\t\t\t\t\
<tt:Encoding>%s</tt:Encoding>\n\t\t\t\t\
<tt:Resolution>\n\t\t\t\t\t\
<tt:Width>%d</tt:Width>\n\t\t\t\t\t\
<tt:Height>%d</tt:Height>\n\t\t\t\t\
</tt:Resolution>\n\t\t\t\t\
<tt:Quality>%d</tt:Quality>\n\t\t\t\t\
<tt:RateControl ConstantBitRate=\"%s\">\n\t\t\t\t\t\
<tt:FrameRateLimit>%d</tt:FrameRateLimit>\n\t\t\t\t\t\
<tt:BitrateLimit>%d</tt:BitrateLimit>\n\t\t\t\t\
</tt:RateControl>\n\t\t\t\t\
<tt:Multicast>\n\t\t\t\t\t\
<tt:Address>\n\t\t\t\t\t\t\
<tt:Type>IPv4</tt:Type>\n\t\t\t\t\t\t\
<tt:IPv4Address>238.255.0.2</tt:IPv4Address>\n\t\t\t\t\t\
</tt:Address>\n\t\t\t\t\t\
<tt:Port>28080</tt:Port>\n\t\t\t\t\t\
<tt:TTL>255</tt:TTL>\n\t\t\t\t\t\
<tt:AutoStart>false</tt:AutoStart>\n\t\t\t\t\
</tt:Multicast>\n\t\t\t\t\
</%s:Configurations>\n\t\t\
";
const char *ONVIF_HTTP_GET_VIDEO_ENCODER_CONFIGURATIONS  =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:GetVideoEncoderConfigurationsResponse>\n\t\t\t\
%s%s\
</%s:GetVideoEncoderConfigurationsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GET_AUDIOENCODER_CONFIGURATION_OPTIONS =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:GetAudioEncoderConfigurationOptionsResponse>\n\t\t\t\
<%s:Options>\n\t\t\t\t\
<tt:Options>\n\t\t\t\t\t\
<tt:Encoding>G711</tt:Encoding>\n\t\t\t\t\t\
<tt:BitrateList>\n\t\t\t\t\t\t\
<tt:Items>64</tt:Items>\n\t\t\t\t\t\
</tt:BitrateList>\n\t\t\t\t\t\
<tt:SampleRateList>\n\t\t\t\t\t\t\
<tt:Items>8</tt:Items>\n\t\t\t\t\t\
</tt:SampleRateList>\n\t\t\t\t\
</tt:Options>\n\t\t\t\
</%s:Options>\n\t\t\
</%s:GetAudioEncoderConfigurationOptionsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

const char *ONVIF_HTTP_GET_AUDIOENCODER_CONFIGURATION =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:GetAudioEncoderConfigurationResponse>\n\t\t\t\
<%s:Configuration token=\"AudioEncoder%sToken_1\">\n\t\t\t\t\
<tt:Name>FirstAudio</tt:Name>\n\t\t\t\t\
<tt:UseCount>3</tt:UseCount>\n\t\t\t\t\
<tt:Encoding>G711</tt:Encoding>\n\t\t\t\t\
<tt:Bitrate>64</tt:Bitrate>\n\t\t\t\t\
<tt:SampleRate>8</tt:SampleRate>\n\t\t\t\t\
<tt:Multicast>\n\t\t\t\t\t\
<tt:Address>\n\t\t\t\t\t\t\
<tt:Type>IPv4</tt:Type>\n\t\t\t\t\t\t\
<tt:IPv4Address>238.255.0.5</tt:IPv4Address>\n\t\t\t\t\t\
</tt:Address>\n\t\t\t\t\t\
<tt:Port>28080</tt:Port>\n\t\t\t\t\t\
<tt:TTL>255</tt:TTL>\n\t\t\t\t\t\
<tt:AutoStart>true</tt:AutoStart>\n\t\t\t\t\
</tt:Multicast>\n\t\t\t\t\
<tt:SessionTimeout>PT0S</tt:SessionTimeout>\n\t\t\t\
</%s:Configuration>\n\t\t\
</%s:GetAudioEncoderConfigurationResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

const char *ONVIF_HTTP_GET_AUDIOENCODER_CONFIGURATIONS =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:GetAudioEncoderConfigurationsResponse>\n\t\t\t\
<%s:Configurations token=\"AudioEncoder%sToken_1\">\n\t\t\t\t\
<tt:Name>FirstAudio</tt:Name>\n\t\t\t\t\
<tt:UseCount>3</tt:UseCount>\n\t\t\t\t\
<tt:Encoding>G711</tt:Encoding>\n\t\t\t\t\
<tt:Bitrate>64</tt:Bitrate>\n\t\t\t\t\
<tt:SampleRate>8</tt:SampleRate>\n\t\t\t\t\
<tt:Multicast>\n\t\t\t\t\t\
<tt:Address>\n\t\t\t\t\t\t\
<tt:Type>IPv4</tt:Type>\n\t\t\t\t\t\t\
<tt:IPv4Address>238.255.0.5</tt:IPv4Address>\n\t\t\t\t\t\
</tt:Address>\n\t\t\t\t\t\
<tt:Port>28080</tt:Port>\n\t\t\t\t\t\
<tt:TTL>255</tt:TTL>\n\t\t\t\t\t\
<tt:AutoStart>false</tt:AutoStart>\n\t\t\t\t\
</tt:Multicast>\n\t\t\t\t\
<tt:SessionTimeout>PT0S</tt:SessionTimeout>\n\t\t\t\
</%s:Configurations>\n\t\t\
</%s:GetAudioEncoderConfigurationsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

const char *ONVIF_HTTP_GETMOVEOPTIONS =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<timg:GetMoveOptionsResponse>\n\t\t\t\
<timg:MoveOptions>\n\t\t\t\t\
<tt:Absolute>\n\t\t\t\t\t\
<tt:Position>\n\t\t\t\t\t\t\
<tt:Min>0</tt:Min>\n\t\t\t\t\t\t\
<tt:Max>1</tt:Max>\n\t\t\t\t\t\
</tt:Position>\n\t\t\t\t\t\
<tt:Speed>\n\t\t\t\t\t\t\
<tt:Min>0</tt:Min>\n\t\t\t\t\t\t\
<tt:Max>1</tt:Max>\n\t\t\t\t\t\
</tt:Speed>\n\t\t\t\t\
</tt:Absolute>\n\t\t\t\t\
<tt:Relative>\n\t\t\t\t\t\
<tt:Distance>\n\t\t\t\t\t\t\
<tt:Min>-1</tt:Min>\n\t\t\t\t\t\t\
<tt:Max>1</tt:Max>\n\t\t\t\t\t\
</tt:Distance>\n\t\t\t\t\t\
<tt:Speed>\n\t\t\t\t\t\t\
<tt:Min>0</tt:Min>\n\t\t\t\t\t\t\
<tt:Max>1</tt:Max>\n\t\t\t\t\t\
</tt:Speed>\n\t\t\t\t\
</tt:Relative>\n\t\t\t\t\
<tt:Continuous>\n\t\t\t\t\t\
<tt:Speed>\n\t\t\t\t\t\t\
<tt:Min>-1</tt:Min>\n\t\t\t\t\t\t\
<tt:Max>1</tt:Max>\n\t\t\t\t\t\
</tt:Speed>\n\t\t\t\t\
</tt:Continuous>\n\t\t\t\
</timg:MoveOptions>\n\t\t\
</timg:GetMoveOptionsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

const char *ONVIF_HTTP_GET_VIDEOANALYTICSCONFIGURATIONS =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<trt:GetVideoAnalyticsConfigurationsResponse>\n\t\t\t\
<trt:Configurations token=\"VideoAnalytics_1\">\n\t\t\t\t\
<tt:Name>VideoAnalytics1</tt:Name>\n\t\t\t\t\
<tt:UseCount>6</tt:UseCount>\n\t\t\t\t\
<tt:AnalyticsEngineConfiguration>\n\t\t\t\t\t\
<tt:AnalyticsModule Type=\"tt:CellMotionEngine\" Name=\"MyCellMotionModule\">\n\t\t\t\t\t\t\
<tt:Parameters>\n\t\t\t\t\t\t\t\
<tt:SimpleItem Name=\"Sensitivity\" Value=\"%d\"></tt:SimpleItem>\n\t\t\t\t\t\t\t\
<tt:ElementItem Name=\"Layout\"><tt:CellLayout Columns=\"22\" Rows=\"18\"><tt:Transformation><tt:Translate x=\"-1.000000\" y=\"-1.000000\"/><tt:Scale x=\"0.045455\" y=\"-0.055556\"/></tt:Transformation></tt:CellLayout></tt:ElementItem>\n\t\t\t\t\t\t\
</tt:Parameters>\n\t\t\t\t\t\
</tt:AnalyticsModule>\n\t\t\t\t\
</tt:AnalyticsEngineConfiguration>\n\t\t\t\t\
<tt:RuleEngineConfiguration>\n\t\t\t\t\t\
<tt:Rule Type=\"tt:CellMotionDetector\" Name=\"MyCellMotionDetectorRule\">\n\t\t\t\t\t\t\
<tt:Parameters>\n\t\t\t\t\t\t\t\
<tt:SimpleItem Name=\"MinCount\" Value=\"5\"></tt:SimpleItem>\n\t\t\t\t\t\t\t\
<tt:SimpleItem Name=\"AlarmOnDelay\" Value=\"1000\"></tt:SimpleItem>\n\t\t\t\t\t\t\t\
<tt:SimpleItem Name=\"AlarmOffDelay\" Value=\"1000\"></tt:SimpleItem>\n\t\t\t\t\t\t\t\
<tt:SimpleItem Name=\"ActiveCells\" Value=\"%s\"></tt:SimpleItem>\n\t\t\t\t\t\t\
</tt:Parameters>\n\t\t\t\t\t\
</tt:Rule>\n\t\t\t\t\
</tt:RuleEngineConfiguration>\n\t\t\t\
</trt:Configurations>\n\t\t\
</trt:GetVideoAnalyticsConfigurationsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

const char *ONVIF_HTTP_SET_SYSTEMDATEANDTIME =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tds:SetSystemDateAndTime>\n\t\t\
</tds:SetSystemDateAndTime>\n\t\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

const char *ONVIF_HTP_GET_ANALYTICSMODULES =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tan:GetAnalyticsModulesResponse>\n\t\t\t\
<tan:AnalyticsModule Type=\"tt:CellMotionEngine\" Name=\"MyCellMotionModule\">\n\t\t\t\t\
<tt:Parameters>\n\t\t\t\t\t\
<tt:SimpleItem Name=\"Sensitivity\" Value=\"%d\"></tt:SimpleItem>\n\t\t\t\t\t\
<tt:ElementItem Name=\"Layout\">\
<tt:CellLayout Columns=\"22\" Rows=\"18\">\
<tt:Transformation><tt:Translate x=\"-1.000000\" y=\"-1.000000\"/><tt:Scale x=\"0.045455\" y=\"-0.055556\"/></tt:Transformation>\
</tt:CellLayout></tt:ElementItem>\n\t\t\t\t\
</tt:Parameters>\n\t\t\t\
</tan:AnalyticsModule>\n\t\t\
</tan:GetAnalyticsModulesResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";


const char *ONVIF_HTTP_GET_RULES =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tan:GetRulesResponse>\n\t\t\t\
<tan:Rule Type=\"tt:CellMotionDetector\" Name=\"MyCellMotionDetectorRule\">\n\t\t\t\t\
<tt:Parameters>\n\t\t\t\t\t\
<tt:SimpleItem Name=\"MinCount\" Value=\"4\"></tt:SimpleItem>\n\t\t\t\t\t\
<tt:SimpleItem Name=\"AlarmOnDelay\" Value=\"1000\"></tt:SimpleItem>\n\t\t\t\t\t\
<tt:SimpleItem Name=\"AlarmOffDelay\" Value=\"1000\"></tt:SimpleItem>\n\t\t\t\t\t\
<tt:SimpleItem Name=\"ActiveCells\" Value=\"%s\"></tt:SimpleItem>\n\t\t\t\t\
</tt:Parameters>\n\t\t\t\
</tan:Rule>\n\t\t</tan:GetRulesResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

const char *ONVIF_HTTP_SUBSCRIBE =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Header>\n\t\t<wsa5:Action SOAP-ENV:mustUnderstand=\"true\">http://docs.oasis-open.org/wsn/bw-2/NotificationProducer/SubscribeResponse</wsa5:Action>\n\t</SOAP-ENV:Header>\n\t\
<SOAP-ENV:Body>\n\t\t\
<wsnt:SubscribeResponse>\n\t\t\t\
<wsnt:SubscriptionReference>\n\t\t\t\t\
<wsa5:Address>http://%s:%d/onvif/Events/Subscription_%s</wsa5:Address>\n\t\t\t\
</wsnt:SubscriptionReference>\n\t\t\t\
<wsnt:CurrentTime>%s</wsnt:CurrentTime>\n\t\t\t\
<wsnt:TerminationTime>%s</wsnt:TerminationTime>\n\t\t\
</wsnt:SubscribeResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

const char *ONVIF_HTTP_UNSUBSCRIBE =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<wsnt:UnsubscribeResponse>\
</wsnt:UnsubscribeResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

const char *ONVIF_HTTP_RENEW =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Header>\n\t\t<wsa5:Action SOAP-ENV:mustUnderstand=\"true\">http://docs.oasis-open.org/wsn/bw-2/SubscriptionManager/RenewResponse</wsa5:Action>\n\t</SOAP-ENV:Header>\n\t\
<SOAP-ENV:Body>\n\t\t\
<wsnt:RenewResponse>\n\t\t\t\
<wsnt:TerminationTime>%s</wsnt:TerminationTime>\n\t\t\t\
<wsnt:CurrentTime>%s</wsnt:CurrentTime>\n\t\t\
</wsnt:RenewResponse>\n\t</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_NOTIFY_MSG_MOTION =
    "\n\t\t\t\
<wsnt:NotificationMessage>\n\t\t\t\t\
<wsnt:Topic Dialect=\"http://docs.oasis-open.org/wsn/t-1/TopicExpression/Simple\">tns1:VideoSource/MotionAlarm</wsnt:Topic>\n\t\t\t\t\
<wsnt:Message>\
<tt:Message UtcTime=\"%s\" PropertyOperation=\"%s\">\
<tt:Source>\
<tt:SimpleItem Name=\"VideoSourceToken\" Value=\"VideoSource_1\" />\
</tt:Source>\
<tt:Data>\
<tt:SimpleItem Name=\"State\" Value=\"%s\" />\
</tt:Data></tt:Message></wsnt:Message>\n\t\t\t\
</wsnt:NotificationMessage>\n\t\t\t\
<wsnt:NotificationMessage>\n\t\t\t\t\
<wsnt:Topic Dialect=\"http://docs.oasis-open.org/wsn/t-1/TopicExpression/Simple\">tns1:RuleEngine/CellMotionDetector/Motion</wsnt:Topic>\n\t\t\t\t\
<wsnt:Message>\
<tt:Message UtcTime=\"%s\" PropertyOperation=\"%s\">\
<tt:Source>\
<tt:SimpleItem Name=\"VideoSourceConfigurationToken\" Value=\"VideoSourceConfig_1_1\" />\
<tt:SimpleItem Name=\"VideoAnalyticsConfigurationToken\" Value=\"VideoAnalytics_1\" />\
<tt:SimpleItem Name=\"Rule\" Value=\"CellMotionDetector\" />\
</tt:Source>\
<tt:Data>\
<tt:SimpleItem Name=\"IsMotion\" Value=\"%s\" />\
</tt:Data></tt:Message></wsnt:Message>\n\t\t\t\
</wsnt:NotificationMessage>\
\n\t\t";

const char *ONVIF_HTTP_NOTIFY_HEADER =
    "POST /%s HTTP/1.1\r\n\
Host: %s:%d\r\n\
User-Agent: onvifserver\r\n\
Content-Type: application/soap+xml; charset=utf-8; action=\"http://docs.oasis-open.org/wsn/bw-2/NotificationConsumer/Notify\"\r\n\
Content-Length: %d\r\n\
Connection: close\r\n\
SOAPAction: \"http://docs.oasis-open.org/wsn/bw-2/NotificationConsumer/Notify\"\r\n\
\r\n\
";
const char *ONVIF_HTTP_NOTIFY_BODY = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" xmlns:wsa5=\"http://www.w3.org/2005/08/addressing\" xmlns:xmime=\"http://tempuri.org/xmime.xsd\" xmlns:xop=\"http://www.w3.org/2004/08/xop/include\" xmlns:wsbf2=\"http://docs.oasis-open.org/wsrf/bf-2\" xmlns:wstop=\"http://docs.oasis-open.org/wsn/t-1\" xmlns:tt=\"http://www.onvif.org/ver10/schema\" xmlns:wsr2=\"http://docs.oasis-open.org/wsrf/r-2\" xmlns:tev=\"http://www.onvif.org/ver10/events/wsdl\" xmlns:depsm=\"http://www.onvif.org/ver10/events/wsdl/PausableSubscriptionManagerBinding\" xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\" xmlns:desm=\"http://www.onvif.org/ver10/events/wsdl/SubscriptionManagerBinding\" xmlns:ter=\"http://www.onvif.org/ver10/error\" xmlns:tns1=\"http://www.onvif.org/ver10/topics\" xmlns:decpp=\"http://www.onvif.org/ver10/events/wsdl/CreatePullPointBinding\" xmlns:dee=\"http://www.onvif.org/ver10/events/wsdl/EventBinding\" xmlns:denc=\"http://www.onvif.org/ver10/events/wsdl/NotificationConsumerBinding\" xmlns:denf=\"http://www.onvif.org/ver10/events/wsdl/NotificationProducerBinding\" xmlns:depp=\"http://www.onvif.org/ver10/events/wsdl/PullPointBinding\" xmlns:depps=\"http://www.onvif.org/ver10/events/wsdl/PullPointSubscriptionBinding\">\n\t\
<SOAP-ENV:Header>\n\t\t\
<wsa5:To SOAP-ENV:mustUnderstand=\"true\">http://%s:%d/%s</wsa5:To>\n\t\t\
<wsa5:Action SOAP-ENV:mustUnderstand=\"true\">http://docs.oasis-open.org/wsn/bw-2/NotificationConsumer/Notify</wsa5:Action>\n\t\
</SOAP-ENV:Header>\n\t\
<SOAP-ENV:Body>\n\t\t\
<wsnt:Notify>%s</wsnt:Notify>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

const char *ONVIF_HTTP_GetEventProperties = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?><SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" xmlns:wsa5=\"http://www.w3.org/2005/08/addressing\" xmlns:xmime=\"http://tempuri.org/xmime.xsd\" xmlns:xop=\"http://www.w3.org/2004/08/xop/include\" xmlns:wsbf2=\"http://docs.oasis-open.org/wsrf/bf-2\" xmlns:wstop=\"http://docs.oasis-open.org/wsn/t-1\" xmlns:tt=\"http://www.onvif.org/ver10/schema\" xmlns:wsr2=\"http://docs.oasis-open.org/wsrf/r-2\" xmlns:decpp=\"http://www.onvif.org/ver10/events/wsdl/CreatePullPointBinding\" xmlns:dee=\"http://www.onvif.org/ver10/events/wsdl/EventBinding\" xmlns:denc=\"http://www.onvif.org/ver10/events/wsdl/NotificationConsumerBinding\" xmlns:denf=\"http://www.onvif.org/ver10/events/wsdl/NotificationProducerBinding\" xmlns:depp=\"http://www.onvif.org/ver10/events/wsdl/PullPointBinding\" xmlns:depps=\"http://www.onvif.org/ver10/events/wsdl/PullPointSubscriptionBinding\" xmlns:tev=\"http://www.onvif.org/ver10/events/wsdl\" xmlns:depsm=\"http://www.onvif.org/ver10/events/wsdl/PausableSubscriptionManagerBinding\" xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\" xmlns:desm=\"http://www.onvif.org/ver10/events/wsdl/SubscriptionManagerBinding\" xmlns:ter=\"http://www.onvif.org/ver10/error\" xmlns:tns1=\"http://www.onvif.org/ver10/topics\">\
<SOAP-ENV:Header><wsa5:Action SOAP-ENV:mustUnderstand=\"true\">http://www.onvif.org/ver10/events/wsdl/EventPortType/GetEventPropertiesResponse</wsa5:Action></SOAP-ENV:Header><SOAP-ENV:Body><tev:GetEventPropertiesResponse><tev:TopicNamespaceLocation>http://www.onvif.org/onvif/ver10/topics/topicns.xml</tev:TopicNamespaceLocation><wsnt:FixedTopicSet>true</wsnt:FixedTopicSet><wstop:TopicSet><tns1:Device><Trigger><DigitalInput wstop:topic=\"true\"><tt:MessageDescription IsProperty=\"true\"><tt:Source><tt:SimpleItemDescription Name=\"InputToken\" Type=\"tt:ReferenceToken\"/></tt:Source><tt:Data><tt:SimpleItemDescription Name=\"LogicalState\" Type=\"xsd:boolean\"/></tt:Data></tt:MessageDescription></DigitalInput><Relay wstop:topic=\"true\"><tt:MessageDescription IsProperty=\"true\"><tt:Source><tt:SimpleItemDescription Name=\"RelayToken\" Type=\"tt:ReferenceToken\"/></tt:Source><tt:Data><tt:SimpleItemDescription Name=\"LogicalState\" Type=\"tt:RelayLogicalState\"/></tt:Data></tt:MessageDescription></Relay></Trigger></tns1:Device><tns1:VideoSource><MotionAlarm wstop:topic=\"true\"><tt:MessageDescription IsProperty=\"true\"><tt:Source><tt:SimpleItemDescription Name=\"VideoSourceToken\" Type=\"tt:ReferenceToken\"/></tt:Source><tt:Data><tt:SimpleItemDescription Name=\"State\" Type=\"xsd:boolean\"/></tt:Data></tt:MessageDescription></MotionAlarm></tns1:VideoSource><tns1:RuleEngine><CellMotionDetector><Motion wstop:topic=\"true\"><tt:MessageDescription IsProperty=\"true\">\
<tt:Source><tt:SimpleItemDescription Name=\"VideoSourceConfigurationToken\" Type=\"tt:ReferenceToken\"/><tt:SimpleItemDescription Name=\"VideoAnalyticsConfigurationToken\" Type=\"tt:ReferenceToken\"/><tt:SimpleItemDescription Name=\"Rule\" Type=\"xsd:string\"/></tt:Source><tt:Data><tt:SimpleItemDescription Name=\"IsMotion\" Type=\"xsd:boolean\"/></tt:Data></tt:MessageDescription></Motion></CellMotionDetector></tns1:RuleEngine></wstop:TopicSet><wsnt:TopicExpressionDialect>http://docs.oasis-open.org/wsn/t-1/TopicExpression/Concrete</wsnt:TopicExpressionDialect><wsnt:TopicExpressionDialect>http://www.onvif.org/ver10/tev/topicExpression/ConcreteSet</wsnt:TopicExpressionDialect><wsnt:TopicExpressionDialect>http://docs.oasis-open.org/wsn/t-1/TopicExpression/Simple</wsnt:TopicExpressionDialect><tev:MessageContentFilterDialect>http://www.onvif.org/ver10/tev/messageContentFilter/ItemFilter</tev:MessageContentFilterDialect><tev:MessageContentSchemaLocation>http://www.onvif.org/onvif/ver10/schema/onvif.xsd</tev:MessageContentSchemaLocation></tev:GetEventPropertiesResponse></SOAP-ENV:Body></SOAP-ENV:Envelope>\r\n\
";

const char *ONVIF_HTTP_CreatePullPointSubscription = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Header>\n\t\t\
<wsa5:Action SOAP-ENV:mustUnderstand=\"true\">http://www.onvif.org/ver10/events/wsdl/EventPortType/CreatePullPointSubscriptionResponse</wsa5:Action>\n\t\
</SOAP-ENV:Header>\n\t\
<SOAP-ENV:Body>\n\t\t\
<tev:CreatePullPointSubscriptionResponse>\n\t\t\t\
<tev:SubscriptionReference>\n\t\t\t\t\
<wsa5:Address>http://%s:%d/onvif/Events/PullPoint_%s</wsa5:Address>\n\t\t\t\
</tev:SubscriptionReference>\n\t\t\t\
<wsnt:CurrentTime>%s</wsnt:CurrentTime>\n\t\t\t\
<wsnt:TerminationTime>%s</wsnt:TerminationTime>\n\t\t\
</tev:CreatePullPointSubscriptionResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

const char *ONVIF_HTTP_PullMessage_BODY = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Header>\n\t\t\
<wsa5:Action SOAP-ENV:mustUnderstand=\"true\">http://www.onvif.org/ver10/events/wsdl/PullPointSubscription/PullMessagesResponse</wsa5:Action>\n\t\
</SOAP-ENV:Header>\n\t\
<SOAP-ENV:Body>\n\t\t\
<tev:PullMessagesResponse>\
<tev:CurrentTime>%s</tev:CurrentTime>\
<tev:TerminationTime>%s</tev:TerminationTime>\
%s\
</tev:PullMessagesResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

const char *ONVIF_HTTP_GetAudioSources_BODY = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<trt:GetAudioSourcesResponse>\n\t\t\t\
<trt:AudioSources token=\"AudioSource_1\">\n\t\t\t\t\
<tt:Channels>1</tt:Channels>\n\t\t\t\
</trt:AudioSources>\n\t\t\
</trt:GetAudioSourcesResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

const char *ONVIF_HTTP_GetAudioSourceConfigurationOptions_BODY = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:GetAudioSourceConfigurationOptionsResponse>\
<%s:Options>\
<tt:InputTokensAvailable>AudioSource_1</tt:InputTokensAvailable>\
</%s:Options>\
</%s:GetAudioSourceConfigurationOptionsResponse>\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GetAudioSourceConfigurations_BODY = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:GetAudioSourceConfigurationsResponse>\n\t\t\t\
<%s:Configurations token=\"AudioSourceConfig_1\">\n\t\t\t\t\
<tt:Name>AudioSourceConfig1</tt:Name>\n\t\t\t\t\
<tt:UseCount>6</tt:UseCount>\n\t\t\t\t\
<tt:SourceToken>AudioSource_1</tt:SourceToken>\n\t\t\t\
</%s:Configurations>\n\t\t\
</%s:GetAudioSourceConfigurationsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

const char *ONVIF_HTTP_GetMetadataConfigurations_BODY = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:GetMetadataConfigurationsResponse>\
</%s:GetMetadataConfigurationsResponse>\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GetAudioOutputConfigurationOptions_BODY = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:GetAudioOutputConfigurationOptionsResponse>\n\t\t\t\
<%s:Options>\n\t\t\t\t\
<tt:OutputTokensAvailable>AudioOutput_1</tt:OutputTokensAvailable>\n\t\t\t\t\
<tt:SendPrimacyOptions>www.onvif.org/ver20/HalfDuplex/Client</tt:SendPrimacyOptions>\n\t\t\t\t\
<tt:OutputLevelRange>\n\t\t\t\t\t\
<tt:Min>0</tt:Min>\n\t\t\t\t\t\
<tt:Max>100</tt:Max>\n\t\t\t\t\
</tt:OutputLevelRange>\n\t\t\t\
</%s:Options>\n\t\t\
</%s:GetAudioOutputConfigurationOptionsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GetAudioOutputConfigurations_BODY = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:GetAudioOutputConfigurationsResponse>\n\t\t\t\
<%s:Configurations token=\"AudioOutputConfig_1\">\n\t\t\t\t\
<tt:Name>AudioOutputConfig1</tt:Name>\n\t\t\t\t\
<tt:UseCount>4</tt:UseCount>\n\t\t\t\t\
<tt:OutputToken>AudioOutput_1</tt:OutputToken>\n\t\t\t\t\
<tt:SendPrimacy>www.onvif.org/ver20/HalfDuplex/Server</tt:SendPrimacy>\n\t\t\t\t\
<tt:OutputLevel>50</tt:OutputLevel>\n\t\t\t\
</%s:Configurations>\n\t\t\
</%s:GetAudioOutputConfigurationsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GetAudioDecoderConfigurationOptions_BODY = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:GetAudioDecoderConfigurationOptionsResponse>\n\t\t\t\
<%s:Options>\n\t\t\t\t\
<tt:Encoding>PCMU</tt:Encoding>\n\t\t\t\t\
<tt:BitrateList>\n\t\t\t\t\t\
<tt:Items>64</tt:Items>\n\t\t\t\t\
</tt:BitrateList>\n\t\t\t\t\
<tt:SampleRateList>\n\t\t\t\t\t\
<tt:Items>8</tt:Items>\n\t\t\t\t\t\
</tt:SampleRateList>\n\t\t\t\
</%s:Options>\n\t\t\
</%s:GetAudioDecoderConfigurationOptionsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GetAudioDecoderConfigurations_BODY = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:GetAudioDecoderConfigurationsResponse>\n\t\t\t\
<%s:Configurations token=\"AudioDecoderToken_1\">\n\t\t\t\t\
<tt:Name>AudioDecoder</tt:Name>\n\t\t\t\t\
<tt:UseCount>4</tt:UseCount>\n\t\t\t\
</%s:Configurations>\n\t\t\
</%s:GetAudioDecoderConfigurationsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GetSnapshotUri_BODY = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<trt:GetSnapshotUriResponse>\n\t\t\t<trt:MediaUri>\n\t\t\t\t<tt:Uri>%s</tt:Uri>\n\t\t\t\t<tt:InvalidAfterConnect>true</tt:InvalidAfterConnect>\n\t\t\t\t<tt:InvalidAfterReboot>true</tt:InvalidAfterReboot>\n\t\t\t\t<tt:Timeout>PT0S</tt:Timeout>\n\t\t\t</trt:MediaUri>\n\t\t</trt:GetSnapshotUriResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

const char *ONVIF_HTTP_GetSnapshotUri_BODY2 = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tr2:GetSnapshotUriResponse>\n\t\t\t<tr2:Uri>%s</tr2:Uri>\n\t\t\</tr2:GetSnapshotUriResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";


const char *ONVIF_HTTP_PushAnalogGpsInfo_BODY = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<extXsd:pushAnalogGpsInfoResponse></extXsd:pushAnalogGpsInfoResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

const char *ONVIF_HTTP_PushStationInfo_BODY = "\
<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<extXsd:pushStationInfoResponse></extXsd:pushStationInfoResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

/* const char *ONVIF_HTTP_PullMessages = "\ */
/* <?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\ */
/* <SOAP-ENV:Envelope %s\n\t\ */
/* <SOAP-ENV:Body>\n\t\t\ */
/* <tev:PullMessagesResponse>\n\t\t\t\ */
/* <tev:CurrentTime>2019-08-05T11:24:31Z</tev:CurrentTime>\n\t\t\t\ */
/* <tev:TerminationTime>2019-08-05T11:36:30Z</tev:TerminationTime>\n\t\t\t\ */
/* <wsnt:NotificationMessage>\n\t\t\t\t\ */
/* <wsnt:Topic Dialect=\"http://docs.oasis-open.org/wsn/t-1/TopicExpression/Simple\">tns1:Device/Trigger/DigitalInput</wsnt:Topic>\n\t\t\t\t\ */
/* <wsnt:Message><tt:Message UtcTime=\"2019-08-08T17:43:31Z\" PropertyOperation=\"Initialized\" ><tt:Source><tt:SimpleItem Name=\"InputToken\" Value=\"AlarmIn01\" /></tt:Source><tt:Key /><tt:Data><tt:SimpleItem Name=\"LogicalState\" Value=\"false\" /></tt:Data></tt:Message></wsnt:Message>\n\t\t\t\ */
/* </wsnt:NotificationMessage>\n\t\t\t\ */
/* <wsnt:NotificationMessage>\n\t\t\t\t\ */
/* <wsnt:Topic Dialect=\"http://docs.oasis-open.org/wsn/t-1/TopicExpression/Simple\">tns1:VideoSource/MotionAlarm</wsnt:Topic>\n\t\t\t\t\ */
/* <wsnt:Message><tt:Message UtcTime=\"2019-08-08T17:43:31Z\" PropertyOperation=\"Initialized\"><tt:Source><tt:SimpleItem Name=\"VideoSourceToken\" Value=\"VideoSource_1\" /></tt:Source><tt:Data><tt:SimpleItem Name=\"State\" Value=\"false\" /></tt:Data></tt:Message></wsnt:Message>\n\t\t\t\ */
/* </wsnt:NotificationMessage>\n\t\t\t\ */
/* <wsnt:NotificationMessage>\n\t\t\t\t\ */
/* <wsnt:Topic Dialect=\"http://docs.oasis-open.org/wsn/t-1/TopicExpression/Simple\">tns1:RuleEngine/CellMotionDetector/Motion</wsnt:Topic>\n\t\t\t\t\ */
/* <wsnt:Message><tt:Message UtcTime=\"2019-08-08T17:43:31Z\" PropertyOperation=\"Initialized\"><tt:Source><tt:SimpleItem Name=\"VideoSourceConfigurationToken\" Value=\"VideoSourceConfig_1_1\" /><tt:SimpleItem Name=\"VideoAnalyticsConfigurationToken\" Value=\"VideoAnalytics_1\" /><tt:SimpleItem Name=\"Rule\" Value=\"CellMotionDetector\" /></tt:Source><tt:Data><tt:SimpleItem Name=\"IsMotion\" Value=\"false\" /></tt:Data></tt:Message></wsnt:Message>\n\t\t\t\ */
/* </wsnt:NotificationMessage>\n\t\t\t\ */
/* <wsnt:NotificationMessage>\n\t\t\t\t\ */
/* <wsnt:Topic Dialect=\"http://docs.oasis-open.org/wsn/t-1/TopicExpression/Simple\">tns1:VideoSource/ImageTooDark</wsnt:Topic>\n\t\t\t\t\ */
/* <wsnt:Message><tt:Message UtcTime=\"2019-08-08T17:43:31Z\" PropertyOperation=\"Initialized\"><tt:Source><tt:SimpleItem Name=\"VideoSourceToken\" Value=\"VideoSource_1\" /></tt:Source><tt:Data><tt:SimpleItem Name=\"State\" Value=\"false\" /></tt:Data></tt:Message></wsnt:Message>\n\t\t\t\ */
/* </wsnt:NotificationMessage>\n\t\t\ */
/* </tev:PullMessagesResponse>\n\t\ */
/* </SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\ */
/* "; */

const char *ONVIF_HTTP_GetOsd_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:GetOSDResponse>\n\t\t\
%s%s%s\
</%s:GetOSDResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GetNetworkProtocols_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tds:GetNetworkProtocolsResponse>\n\t\
<tds:NetworkProtocols>\
<tt:Name>HTTP</tt:Name>\
<tt:Enabled>true</tt:Enabled>\
<tt:Port>80</tt:Port>\
</tds:NetworkProtocols>\
<tds:NetworkProtocols>\
<tt:Name>HTTPS</tt:Name>\
<tt:Enabled>true</tt:Enabled>\
<tt:Port>443</tt:Port>\
</tds:NetworkProtocols>\
<tds:NetworkProtocols>\
<tt:Name>RTSP</tt:Name>\
<tt:Enabled>true</tt:Enabled>\
<tt:Port>554</tt:Port>\
</tds:NetworkProtocols>\
</tds:GetNetworkProtocolsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GetConfigurations_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" \
xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" \
xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" \
xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" \
xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" \
xmlns:wsdd=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" \
xmlns:c14n=\"http://www.w3.org/2001/10/xml-exc-c14n#\" \
xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\" \
xmlns:saml1=\"urn:oasis:names:tc:SAML:1.0:assertion\" \
xmlns:saml2=\"urn:oasis:names:tc:SAML:2.0:assertion\" \
xmlns:wsu=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\" \
xmlns:xenc=\"http://www.w3.org/2001/04/xmlenc#\" \
xmlns:wsc=\"http://docs.oasis-open.org/ws-sx/ws-secureconversation/200512\" \
xmlns:wsse=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd\" \
xmlns:chan=\"http://schemas.microsoft.com/ws/2005/02/duplex\" \
xmlns:wsa5=\"http://www.w3.org/2005/08/addressing\" \
xmlns:xmime=\"http://tempuri.org/xmime.xsd\" \
xmlns:xop=\"http://www.w3.org/2004/08/xop/include\" \
xmlns:tt=\"http://www.onvif.org/ver10/schema\" \
xmlns:wsrfbf=\"http://docs.oasis-open.org/wsrf/bf-2\" \
xmlns:wstop=\"http://docs.oasis-open.org/wsn/t-1\" \
xmlns:wsrfr=\"http://docs.oasis-open.org/wsrf/r-2\" \
xmlns:tan=\"http://www.onvif.org/ver20/analytics/wsdl\" \
xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" \
xmlns:tev=\"http://www.onvif.org/ver10/events/wsdl\" \
xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\" \
xmlns:timg=\"http://www.onvif.org/ver20/imaging/wsdl\" \
xmlns:tmd=\"http://www.onvif.org/ver10/deviceIO/wsdl\" \
xmlns:tptz=\"http://www.onvif.org/ver20/ptz/wsdl\" \
xmlns:tr2=\"http://www.onvif.org/ver20/media/wsdl\" \
xmlns:trt=\"http://www.onvif.org/ver10/media/wsdl\" \
xmlns:tns1=\"http://www.onvif.org/ver10/topics\" \
xmlns:ter=\"http://www.onvif.org/ver10/error\" \
xmlns:extxsd=\"http://www.onvifext.com/onvif/ext/ver10/schema\">\
<SOAP-ENV:Body>\
<tptz:GetConfiguration%sResponse>\
<tptz:PTZConfiguration token=\"PTZConfig_1\">\
<tt:Name>PTZConfig1</tt:Name>\
<tt:UseCount>6</tt:UseCount>\
<tt:NodeToken>PTZNode_0</tt:NodeToken>\
<tt:DefaultAbsolutePantTiltPositionSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:DefaultAbsolutePantTiltPositionSpace>\
<tt:DefaultAbsoluteZoomPositionSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:DefaultAbsoluteZoomPositionSpace>\
<tt:DefaultRelativePanTiltTranslationSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:DefaultRelativePanTiltTranslationSpace>\
<tt:DefaultRelativeZoomTranslationSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:DefaultRelativeZoomTranslationSpace>\
<tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace>\
<tt:DefaultContinuousZoomVelocitySpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:DefaultContinuousZoomVelocitySpace>\
<tt:DefaultPTZSpeed>\
<tt:PanTilt x=\"1\" y=\"1\" space=\"http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace\" />\
<tt:Zoom x=\"1\" space=\"http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace\" />\
</tt:DefaultPTZSpeed>\
<tt:DefaultPTZTimeout>PT60S</tt:DefaultPTZTimeout>\
<tt:PanTiltLimits>\
<tt:Range>\
<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:URI>\
<tt:XRange>\
<tt:Min>-1</tt:Min>\
<tt:Max>1</tt:Max>\
</tt:XRange>\
<tt:YRange>\
<tt:Min>-1</tt:Min>\
<tt:Max>1</tt:Max>\
</tt:YRange>\
</tt:Range>\
</tt:PanTiltLimits>\
<tt:ZoomLimits>\
<tt:Range>\
<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:URI>\
<tt:XRange>\
<tt:Min>-1</tt:Min>\
<tt:Max>1</tt:Max>\
</tt:XRange>\
</tt:Range>\
</tt:ZoomLimits>\
</tptz:PTZConfiguration>\
</tptz:GetConfiguration%sResponse>\
</SOAP-ENV:Body>\
</SOAP-ENV:Envelope>";

const char *ONVIF_HTTP_GetConfigurationOptions_BODY =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" xmlns:wsdd=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" xmlns:c14n=\"http://www.w3.org/2001/10/xml-exc-c14n#\" xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\" xmlns:saml1=\"urn:oasis:names:tc:SAML:1.0:assertion\" xmlns:saml2=\"urn:oasis:names:tc:SAML:2.0:assertion\" xmlns:wsu=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\" xmlns:xenc=\"http://www.w3.org/2001/04/xmlenc#\" xmlns:wsc=\"http://docs.oasis-open.org/ws-sx/ws-secureconversation/200512\" xmlns:wsse=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd\" xmlns:chan=\"http://schemas.microsoft.com/ws/2005/02/duplex\" xmlns:wsa5=\"http://www.w3.org/2005/08/addressing\" xmlns:xmime=\"http://tempuri.org/xmime.xsd\" xmlns:xop=\"http://www.w3.org/2004/08/xop/include\" xmlns:tt=\"http://www.onvif.org/ver10/schema\" xmlns:wsrfbf=\"http://docs.oasis-open.org/wsrf/bf-2\" xmlns:wstop=\"http://docs.oasis-open.org/wsn/t-1\" xmlns:wsrfr=\"http://docs.oasis-open.org/wsrf/r-2\" xmlns:tan=\"http://www.onvif.org/ver20/analytics/wsdl\" xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" xmlns:tev=\"http://www.onvif.org/ver10/events/wsdl\" xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\" xmlns:timg=\"http://www.onvif.org/ver20/imaging/wsdl\" xmlns:tmd=\"http://www.onvif.org/ver10/deviceIO/wsdl\" xmlns:tptz=\"http://www.onvif.org/ver20/ptz/wsdl\" xmlns:tr2=\"http://www.onvif.org/ver20/media/wsdl\" xmlns:trt=\"http://www.onvif.org/ver10/media/wsdl\" xmlns:tns1=\"http://www.onvif.org/ver10/topics\" xmlns:ter=\"http://www.onvif.org/ver10/error\" xmlns:extxsd=\"http://www.onvifext.com/onvif/ext/ver10/schema\"><SOAP-ENV:Body><tptz:GetConfigurationOptionsResponse><tptz:PTZConfigurationOptions><tt:Spaces><tt:AbsolutePanTiltPositionSpace><tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange><tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange></tt:AbsolutePanTiltPositionSpace><tt:AbsoluteZoomPositionSpace><tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange></tt:AbsoluteZoomPositionSpace><tt:RelativePanTiltTranslationSpace><tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange><tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange></tt:RelativePanTiltTranslationSpace><tt:RelativeZoomTranslationSpace><tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange></tt:RelativeZoomTranslationSpace><tt:ContinuousPanTiltVelocitySpace><tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange><tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange></tt:ContinuousPanTiltVelocitySpace><tt:ContinuousZoomVelocitySpace><tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange></tt:ContinuousZoomVelocitySpace><tt:PanTiltSpeedSpace><tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange></tt:PanTiltSpeedSpace><tt:ZoomSpeedSpace><tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange></tt:ZoomSpeedSpace></tt:Spaces><tt:PTZTimeout><tt:Min>PT00H00M01S</tt:Min><tt:Max>PT00H01M00S</tt:Max></tt:PTZTimeout></tptz:PTZConfigurationOptions></tptz:GetConfigurationOptionsResponse></SOAP-ENV:Body></SOAP-ENV:Envelope>";

const char *ONVIF_HTTP_GetNode_BODY =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" \
xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" \
xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" \
xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" \
xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" \
xmlns:wsdd=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" \
xmlns:c14n=\"http://www.w3.org/2001/10/xml-exc-c14n#\" \
xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\" \
xmlns:saml1=\"urn:oasis:names:tc:SAML:1.0:assertion\" \
xmlns:saml2=\"urn:oasis:names:tc:SAML:2.0:assertion\" \
xmlns:wsu=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\" \
xmlns:xenc=\"http://www.w3.org/2001/04/xmlenc#\" \
xmlns:wsc=\"http://docs.oasis-open.org/ws-sx/ws-secureconversation/200512\" \
xmlns:wsse=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd\" \
xmlns:chan=\"http://schemas.microsoft.com/ws/2005/02/duplex\" \
xmlns:wsa5=\"http://www.w3.org/2005/08/addressing\" \
xmlns:xmime=\"http://tempuri.org/xmime.xsd\" \
xmlns:xop=\"http://www.w3.org/2004/08/xop/include\" \
xmlns:tt=\"http://www.onvif.org/ver10/schema\" \
xmlns:wsrfbf=\"http://docs.oasis-open.org/wsrf/bf-2\" \
xmlns:wstop=\"http://docs.oasis-open.org/wsn/t-1\" \
xmlns:wsrfr=\"http://docs.oasis-open.org/wsrf/r-2\" \
xmlns:tan=\"http://www.onvif.org/ver20/analytics/wsdl\" \
xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" \
xmlns:tev=\"http://www.onvif.org/ver10/events/wsdl\" \
xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\" \
xmlns:timg=\"http://www.onvif.org/ver20/imaging/wsdl\" \
xmlns:tmd=\"http://www.onvif.org/ver10/deviceIO/wsdl\" \
xmlns:tptz=\"http://www.onvif.org/ver20/ptz/wsdl\" \
xmlns:tr2=\"http://www.onvif.org/ver20/media/wsdl\" \
xmlns:trt=\"http://www.onvif.org/ver10/media/wsdl\" \
xmlns:tns1=\"http://www.onvif.org/ver10/topics\" \
xmlns:ter=\"http://www.onvif.org/ver10/error\" \
xmlns:extxsd=\"http://www.onvifext.com/onvif/ext/ver10/schema\">\
<SOAP-ENV:Body>\
<tptz:GetNodeResponse>\
<tptz:PTZNode token=\"ptz0\"><tt:Name>PTZNode</tt:Name><tt:SupportedPTZSpaces>\
<tt:AbsolutePanTiltPositionSpace>\
<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:URI>\
<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>\
<tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange>\
</tt:AbsolutePanTiltPositionSpace><tt:AbsoluteZoomPositionSpace>\
<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:URI>\
<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>\
</tt:AbsoluteZoomPositionSpace><tt:RelativePanTiltTranslationSpace>\
<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:URI>\
<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>\
<tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange>\
</tt:RelativePanTiltTranslationSpace><tt:RelativeZoomTranslationSpace>\
<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:URI>\
<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>\
</tt:RelativeZoomTranslationSpace>\
<tt:ContinuousPanTiltVelocitySpace>\
<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:URI>\
<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>\
<tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange>\
</tt:ContinuousPanTiltVelocitySpace><tt:ContinuousZoomVelocitySpace>\
<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:URI>\
<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>\
</tt:ContinuousZoomVelocitySpace><tt:PanTiltSpeedSpace>\
<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace</tt:URI>\
<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>\
</tt:PanTiltSpeedSpace>\
	<tt:ZoomSpeedSpace>\
	<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace</tt:URI>\
	<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>\
	</tt:ZoomSpeedSpace>\
</tt:SupportedPTZSpaces>\
<tt:MaximumNumberOfPresets>255</tt:MaximumNumberOfPresets>\
<tt:HomeSupported>false</tt:HomeSupported>\
<tt:AuxiliaryCommands>tt:Wiper|On</tt:AuxiliaryCommands>\
<tt:AuxiliaryCommands>tt:Wiper|Off</tt:AuxiliaryCommands>\
<tt:AuxiliaryCommands>tt:Lamp|On</tt:AuxiliaryCommands>\
<tt:AuxiliaryCommands>tt:Lamp|Off</tt:AuxiliaryCommands>\
<tt:AuxiliaryCommands>tt:Diaphragm|Large</tt:AuxiliaryCommands>\
<tt:AuxiliaryCommands>tt:Diaphragm|Small</tt:AuxiliaryCommands>\
</tptz:PTZNode>\
</tptz:GetNodeResponse>\
</SOAP-ENV:Body>\
</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GetNodes_BODY =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" xmlns:wsdd=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" xmlns:c14n=\"http://www.w3.org/2001/10/xml-exc-c14n#\" xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\" xmlns:saml1=\"urn:oasis:names:tc:SAML:1.0:assertion\" xmlns:saml2=\"urn:oasis:names:tc:SAML:2.0:assertion\" xmlns:wsu=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\" xmlns:xenc=\"http://www.w3.org/2001/04/xmlenc#\" xmlns:wsc=\"http://docs.oasis-open.org/ws-sx/ws-secureconversation/200512\" xmlns:wsse=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd\" xmlns:chan=\"http://schemas.microsoft.com/ws/2005/02/duplex\" xmlns:wsa5=\"http://www.w3.org/2005/08/addressing\" xmlns:xmime=\"http://tempuri.org/xmime.xsd\" xmlns:xop=\"http://www.w3.org/2004/08/xop/include\" xmlns:tt=\"http://www.onvif.org/ver10/schema\" xmlns:wsrfbf=\"http://docs.oasis-open.org/wsrf/bf-2\" xmlns:wstop=\"http://docs.oasis-open.org/wsn/t-1\" xmlns:wsrfr=\"http://docs.oasis-open.org/wsrf/r-2\" xmlns:tan=\"http://www.onvif.org/ver20/analytics/wsdl\" xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" xmlns:tev=\"http://www.onvif.org/ver10/events/wsdl\" xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\" xmlns:timg=\"http://www.onvif.org/ver20/imaging/wsdl\" xmlns:tmd=\"http://www.onvif.org/ver10/deviceIO/wsdl\" xmlns:tptz=\"http://www.onvif.org/ver20/ptz/wsdl\" xmlns:tr2=\"http://www.onvif.org/ver20/media/wsdl\" xmlns:trt=\"http://www.onvif.org/ver10/media/wsdl\" xmlns:tns1=\"http://www.onvif.org/ver10/topics\" xmlns:ter=\"http://www.onvif.org/ver10/error\" xmlns:extxsd=\"http://www.onvifext.com/onvif/ext/ver10/schema\">\
<SOAP-ENV:Body><tptz:GetNodesResponse><tptz:PTZNode token=\"ptz0\"><tt:Name>ptz0</tt:Name><tt:SupportedPTZSpaces><tt:AbsolutePanTiltPositionSpace>\
<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange><tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange></tt:AbsolutePanTiltPositionSpace><tt:AbsoluteZoomPositionSpace><tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange></tt:AbsoluteZoomPositionSpace><tt:RelativePanTiltTranslationSpace><tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange><tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange></tt:RelativePanTiltTranslationSpace><tt:RelativeZoomTranslationSpace><tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange></tt:RelativeZoomTranslationSpace><tt:ContinuousPanTiltVelocitySpace><tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange><tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange></tt:ContinuousPanTiltVelocitySpace><tt:ContinuousZoomVelocitySpace><tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange></tt:ContinuousZoomVelocitySpace><tt:PanTiltSpeedSpace><tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange></tt:PanTiltSpeedSpace><tt:ZoomSpeedSpace><tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace</tt:URI><tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange></tt:ZoomSpeedSpace></tt:SupportedPTZSpaces><tt:MaximumNumberOfPresets>255</tt:MaximumNumberOfPresets><tt:HomeSupported>false</tt:HomeSupported><tt:AuxiliaryCommands>tt:Wiper|On</tt:AuxiliaryCommands>\
<tt:AuxiliaryCommands>tt:Wiper|Off</tt:AuxiliaryCommands><tt:AuxiliaryCommands>tt:Lamp|On</tt:AuxiliaryCommands><tt:AuxiliaryCommands>tt:Lamp|Off</tt:AuxiliaryCommands><tt:AuxiliaryCommands>tt:Diaphragm|Large</tt:AuxiliaryCommands><tt:AuxiliaryCommands>tt:Diaphragm|Small</tt:AuxiliaryCommands></tptz:PTZNode></tptz:GetNodesResponse></SOAP-ENV:Body></SOAP-ENV:Envelope>";

const char *ONVIF_HTTP_PTZ_BODY =
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s>\
%s\
</%s>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GetPresets_ITEM =
"<tptz:Preset token=\"%s\"><tt:Name>%s</tt:Name></tptz:Preset>";
const char *ONVIF_HTTP_SetPreset_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:SetPresetResponse>\
<%s:PresetToken>%d</%s:PresetToken>\
</%s:SetPresetResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GetStatus_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tptz%s:GetStatusResponse>\
<tptz%s:PTZStatus>\
<tt:Position>\
<tt:PanTilt x=\"0.500000\" y=\"0.500000\" space=\"http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace\"/>\
<tt:Zoom x=\"%f\" space=\"http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace\"/>\
</tt:Position>\
<tt:MoveStatus>\
<tt:PanTilt>IDLE</tt:PanTilt>\
<tt:Zoom>IDLE</tt:Zoom>\
</tt:MoveStatus>\
<tt:Error>NO error</tt:Error>\
<tt:UtcTime>%s</tt:UtcTime>\
</tptz%s:PTZStatus>\
</tptz%s:GetStatusResponse>\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_HK_MaskOptions_BODY=
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<env:Envelope xmlns:env=\"http://www.w3.org/2003/05/soap-envelope\" \
xmlns:soapenc=\"http://www.w3.org/2003/05/soap-encoding\" \
xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" \
xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" \
xmlns:tt=\"http://www.onvif.org/ver10/schema\" \
xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" \
xmlns:trt=\"http://www.onvif.org/ver10/media/wsdl\" \
xmlns:timg=\"http://www.onvif.org/ver20/imaging/wsdl\" \
xmlns:tev=\"http://www.onvif.org/ver10/events/wsdl\" \
xmlns:tptz=\"http://www.onvif.org/ver20/ptz/wsdl\" \
xmlns:tan=\"http://www.onvif.org/ver20/analytics/wsdl\" \
xmlns:tst=\"http://www.onvif.org/ver10/storage/wsdl\" \
xmlns:ter=\"http://www.onvif.org/ver10/error\" \
xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\" \
xmlns:tns1=\"http://www.onvif.org/ver10/topics\" \
xmlns:tmd=\"http://www.onvif.org/ver10/deviceIO/wsdl\" \
xmlns:wsdl=\"http://schemas.xmlsoap.org/wsdl\" \
xmlns:wsoap12=\"http://schemas.xmlsoap.org/wsdl/soap12\" \
xmlns:http=\"http://schemas.xmlsoap.org/wsdl/http\" \
xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" \
xmlns:wsadis=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" \
xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\" \
xmlns:wsa=\"http://www.w3.org/2005/08/addressing\" \
xmlns:wstop=\"http://docs.oasis-open.org/wsn/t-1\" \
xmlns:wsrf-bf=\"http://docs.oasis-open.org/wsrf/bf-2\" \
xmlns:wsntw=\"http://docs.oasis-open.org/wsn/bw-2\" \
xmlns:wsrf-rw=\"http://docs.oasis-open.org/wsrf/rw-2\" \
xmlns:wsaw=\"http://www.w3.org/2006/05/addressing/wsdl\" \
xmlns:wsrf-r=\"http://docs.oasis-open.org/wsrf/r-2\" \
xmlns:trc=\"http://www.onvif.org/ver10/recording/wsdl\" \
xmlns:tse=\"http://www.onvif.org/ver10/search/wsdl\" \
xmlns:trp=\"http://www.onvif.org/ver10/replay/wsdl\" \
xmlns:tnsn=\"http://www.eventextension.com/2011/event/topics\" \
xmlns:extwsd=\"http://www.onvifext.com/onvif/ext/ver10/wsdl\" \
xmlns:extxsd=\"http://www.onvifext.com/onvif/ext/ver10/schema\" \
xmlns:tas=\"http://www.onvif.org/ver10/advancedsecurity/wsdl\">\
<env:Body>\
<extwsd:GetPrivacyMaskOptionsResponse>\
<extwsd:PrivacyMaskOptions>\
<extxsd:MaximumNumberOfAreas>4</extxsd:MaximumNumberOfAreas>\
<extxsd:Position>\
<extxsd:XRange><tt:Min>-1.000000</tt:Min><tt:Max>1.000000</tt:Max></extxsd:XRange>\
<extxsd:YRange><tt:Min>-1.000000</tt:Min><tt:Max>1.000000</tt:Max></extxsd:YRange>\
</extxsd:Position>\
</extwsd:PrivacyMaskOptions>\
</extwsd:GetPrivacyMaskOptionsResponse>\
</env:Body>\
</env:Envelope>\
";

const char *ONVIF_HTTP_HK_PrivacyMask_BODY=
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<env:Envelope xmlns:env=\"http://www.w3.org/2003/05/soap-envelope\" \
xmlns:soapenc=\"http://www.w3.org/2003/05/soap-encoding\" \
xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" \
xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" \
xmlns:tt=\"http://www.onvif.org/ver10/schema\" \
xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" \
xmlns:trt=\"http://www.onvif.org/ver10/media/wsdl\" \
xmlns:timg=\"http://www.onvif.org/ver20/imaging/wsdl\" \
xmlns:tev=\"http://www.onvif.org/ver10/events/wsdl\" \
xmlns:tptz=\"http://www.onvif.org/ver20/ptz/wsdl\" \
xmlns:tan=\"http://www.onvif.org/ver20/analytics/wsdl\" \
xmlns:tst=\"http://www.onvif.org/ver10/storage/wsdl\" \
xmlns:ter=\"http://www.onvif.org/ver10/error\" \
xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\" \
xmlns:tns1=\"http://www.onvif.org/ver10/topics\" \
xmlns:tmd=\"http://www.onvif.org/ver10/deviceIO/wsdl\" \
xmlns:wsdl=\"http://schemas.xmlsoap.org/wsdl\" \
xmlns:wsoap12=\"http://schemas.xmlsoap.org/wsdl/soap12\" \
xmlns:http=\"http://schemas.xmlsoap.org/wsdl/http\" \
xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" \
xmlns:wsadis=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" \
xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\" \
xmlns:wsa=\"http://www.w3.org/2005/08/addressing\" \
xmlns:wstop=\"http://docs.oasis-open.org/wsn/t-1\" \
xmlns:wsrf-bf=\"http://docs.oasis-open.org/wsrf/bf-2\" \
xmlns:wsntw=\"http://docs.oasis-open.org/wsn/bw-2\" \
xmlns:wsrf-rw=\"http://docs.oasis-open.org/wsrf/rw-2\" \
xmlns:wsaw=\"http://www.w3.org/2006/05/addressing/wsdl\" \
xmlns:wsrf-r=\"http://docs.oasis-open.org/wsrf/r-2\" \
xmlns:trc=\"http://www.onvif.org/ver10/recording/wsdl\" \
xmlns:tse=\"http://www.onvif.org/ver10/search/wsdl\" \
xmlns:trp=\"http://www.onvif.org/ver10/replay/wsdl\" \
xmlns:tnsn=\"http://www.eventextension.com/2011/event/topics\" \
xmlns:extwsd=\"http://www.onvifext.com/onvif/ext/ver10/wsdl\" \
xmlns:extxsd=\"http://www.onvifext.com/onvif/ext/ver10/schema\" \
xmlns:tas=\"http://www.onvif.org/ver10/advancedsecurity/wsdl\">\
<env:Body>\
<extwsd:GetPrivacyMasksResponse></extwsd:GetPrivacyMasksResponse>\
</env:Body>\
</env:Envelope>\
";

const char *ONVIF_HTTP_GetVideoSourceConfigurations_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<%s:GetVideoSourceConfigurationsResponse>\n\t\t\t\
<%s:Configurations token=\"VideoSourceConfig_1_1\">\n\t\t\t\t\
<tt:Name>VideoSourceConfig1_1</tt:Name>\n\t\t\t\t\
<tt:UseCount>6</tt:UseCount>\n\t\t\t\t\
<tt:SourceToken>VideoSource_1</tt:SourceToken>\n\t\t\t\t\
<tt:Bounds x=\"0\" y=\"0\" width=\"%d\" height=\"%d\"></tt:Bounds>\n\t\t\t\
</%s:Configurations>\n\t\t\
</%s:GetVideoSourceConfigurationsResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GetNTP_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t<tds:GetNTPResponse>\n\t\t\t<tds:NTPInformation>\n\t\t\t\t<tt:FromDHCP>false</tt:FromDHCP>\n\t\t\t\t<tt:NTPManual>\n\t\t\t\t\t<tt:Type>IPv4</tt:Type>\n\t\t\t\t</tt:NTPManual>\n\t\t\t</tds:NTPInformation>\n\t\t</tds:GetNTPResponse>\n\t</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GetDiscoveryMode_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t<tds:GetDiscoveryModeResponse>\n\t\t\t<tds:DiscoveryMode>Discoverable</tds:DiscoveryMode>\n\t\t</tds:GetDiscoveryModeResponse>\n\t</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GetNetworkDefaultGateway_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t<tds:GetNetworkDefaultGatewayResponse>\n\t\t\t<tds:NetworkGateway>\n\t\t\t\t<tt:IPv4Address>%s</tt:IPv4Address>\n\t\t\t</tds:NetworkGateway>\n\t\t</tds:GetNetworkDefaultGatewayResponse>\n\t</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_GetHostname_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t<tds:GetHostnameResponse>\n\t\t\t<tds:HostnameInformation>\n\t\t\t\t<tt:FromDHCP>false</tt:FromDHCP>\n\t\t\t\t<tt:Name>IPC</tt:Name>\n\t\t\t</tds:HostnameInformation>\n\t\t</tds:GetHostnameResponse>\n\t</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_SetSynchronizationPoint_BODY =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tev:SetSynchronizationPointResponse>\n\t\t\
</tev:SetSynchronizationPointResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_SystemReboot_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tds:SystemRebootResponse></tds:SystemRebootResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_SupportedAnalyticsModules_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" \
xmlns:e=\"http://www.w3.org/2003/05/soap-encoding\" \
xmlns:wsa=\"http://www.w3.org/2005/08/addressing\" \
xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" \
xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" \
xmlns:wsaw=\"http://www.w3.org/2006/05/addressing/wsdl\" \
xmlns:wsnt=\"http://docs.oasis-open.org/wsn/b-2\" \
xmlns:wstop=\"http://docs.oasis-open.org/wsn/t-1\" \
xmlns:wsntw=\"http://docs.oasis-open.org/wsn/bw-2\" \
xmlns:wsrf-rw=\"http://docs.oasis-open.org/wsrf/rw-2\" \
xmlns:wsrf-r=\"http://docs.oasis-open.org/wsrf/r-2\" \
xmlns:wsrf-bf=\"http://docs.oasis-open.org/wsrf/bf-2\" \
xmlns:wsdl=\"http://schemas.xmlsoap.org/wsdl\" \
xmlns:wsoap12=\"http://schemas.xmlsoap.org/wsdl/soap12\" \
xmlns:http=\"http://schemas.xmlsoap.org/wsdl/http\" \
xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" \
xmlns:wsadis=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" \
xmlns:tt=\"http://www.onvif.org/ver10/schema\" \
xmlns:tns1=\"http://www.onvif.org/ver10/topics\" \
xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\" \
xmlns:trt=\"http://www.onvif.org/ver10/media/wsdl\" \
xmlns:tev=\"http://www.onvif.org/ver10/events/wsdl\" \
xmlns:timg=\"http://www.onvif.org/ver20/imaging/wsdl\" \
xmlns:tst=\"http://www.onvif.org/ver10/storage/wsdl\" \
xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\" \
xmlns:tr2=\"http://www.onvif.org/ver20/media/wsdl\" \
xmlns:tptz=\"http://www.onvif.org/ver20/ptz/wsdl\" \
xmlns:tan=\"http://www.onvif.org/ver20/analytics/wsdl\" \
xmlns:axt=\"http://www.onvif.org/ver20/analytics\" \
xmlns:tmd=\"http://www.onvif.org/ver10/deviceIO/wsdl\" \
xmlns:ter=\"http://www.onvif.org/ver10/error\">\
<s:Header/>\
<s:Body>\
<tan:GetSupportedAnalyticsModulesResponse>\
<tan:SupportedAnalyticsModules>\
<tt:AnalyticsModuleDescription Name=\"tt:CellMotionEngine\">\
<tt:Parameters>\
<tt:SimpleItemDescription Name=\"Sensitivity\" Type=\"xs:integer\"/>\
<tt:ElementItemDescription Name=\"Layout\" Type=\"tt:CellLayout\"/>\
</tt:Parameters><tt:Messages IsProperty=\"true\">\
<tt:Source>\
<tt:SimpleItemDescription Name=\"VideoSourceConfigurationToken\" Type=\"tt:ReferenceToken\"/>\
<tt:SimpleItemDescription Name=\"VideoAnalyticsConfigurationToken\" Type=\"tt:ReferenceToken\"/>\
<tt:SimpleItemDescription Name=\"Rule\" Type=\"xs:string\"/>\
</tt:Source>\
<tt:Data>\
<tt:SimpleItemDescription Name=\"IsMotion\" Type=\"xs:boolean\"/>\
</tt:Data><tt:ParentTopic>tns1:RuleEngine/CellMotionDetector/Motion</tt:ParentTopic>\
</tt:Messages>\
</tt:AnalyticsModuleDescription>\
</tan:SupportedAnalyticsModules>\
</tan:GetSupportedAnalyticsModulesResponse>\
</s:Body>\
</s:Envelope>\r\n";

    /*"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Header></SOAP-ENV:Header>\n\t\
<SOAP-ENV:Body>\n\t\t\
<tan:GetSupportedAnalyticsModulesResponse>\n\t\t\t\
<tan:SupportedAnalyticsModules>\n\t\t\t\t\
</tan:SupportedAnalyticsModules>\n\t\t\
</tan:GetSupportedAnalyticsModulesResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";*/


    /*"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Header></SOAP-ENV:Header>\n\t\
<SOAP-ENV:Body>\n\t\t\
<tan:GetSupportedAnalyticsModulesResponse>\n\t\t\t\
<tan:SupportedAnalyticsModules>\n\t\t\t\t\
<tt:AnalyticsModuleDescription Name=\"tt:CellMotionEngine\">\n\t\t\t\t\t\
<tt:Parameters>\n\t\t\t\t\t\t\
<tt:SimpleItemDescription Name=\"Sensitivity\" Type=\"xs:integer\" />\n\t\t\t\t\t\t\
<tt:ElementItemDescription Name=\"Layout\" Type=\"tt:CellLayout\" />\n\t\t\t\t\t\
</tt:Parameters>\n\t\t\t\t\t\
<tt:Messages IsProperty=\"true\">\n\t\t\t\t\t\t\
<tt:Source>\n\t\t\t\t\t\t\t\
<tt:SimpleItemDescription Name=\"VideoSourceConfigurationToken\" Type=\"tt:ReferenceToken\" />\n\t\t\t\t\t\t\t\
<tt:SimpleItemDescription Name=\"VideoAnalyticsConfigurationToken\" Type=\"tt:ReferenceToken\" />\n\t\t\t\t\t\t\t\
<tt:SimpleItemDescription Name=\"Rule\" Type=\"xs:string\" />\n\t\t\t\t\t\t\
</tt:Source>\n\t\t\t\t\t\t\
<tt:Data>\n\t\t\t\t\t\t\t\
<tt:SimpleItemDescription Name=\"IsMotion\" Type=\"xs:boolean\" />\n\t\t\t\t\t\t\
</tt:Data>\n\t\t\t\t\t\t\
<tt:ParentTopic>tns1:RuleEngine/CellMotionDetector/Motion</tt:ParentTopic>\n\t\t\t\t\t\
</tt:Messages>\n\t\t\t\t\
</tt:AnalyticsModuleDescription>\n\t\t\t\
</tan:SupportedAnalyticsModules>\n\t\t\
</tan:GetSupportedAnalyticsModulesResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";*/


const char *ONVIF_HTTP_SupportedRules_BODY =
/*    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" \
xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" \
xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" \
xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" \
xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" \
xmlns:wsa5=\"http://www.w3.org/2005/08/addressing\" \
xmlns:tt=\"http://www.onvif.org/ver10/schema\" \
xmlns:tanae=\"http://www.onvif.org/ver20/analytics/wsdl/AnalyticsEngineBinding\" \
xmlns:tanre=\"http://www.onvif.org/ver20/analytics/wsdl/RuleEngineBinding\" \
xmlns:tan=\"http://www.onvif.org/ver20/analytics/wsdl\" \
xmlns:axt=\"http://www.onvif.org/ver20/analytics\" \
xmlns:ter=\"http://www.onvif.org/ver10/error\" \
xmlns:tns1=\"http://www.onvif.org/ver10/topics\">\
<SOAP-ENV:Header></SOAP-ENV:Header>\
<SOAP-ENV:Body>\
<tan:GetSupportedRulesResponse>\
<tan:SupportedRules>\
<tt:RuleContentSchemaLocation>http://www.w3.org/2001/XMLSchema</tt:RuleContentSchemaLocation>\
<tt:RuleDescription Name=\"tt:CellMotionDetector\">\
<tt:Parameters>\
<tt:SimpleItemDescription Name=\"MinCount\" Type=\"xs:integer\" />\
<tt:SimpleItemDescription Name=\"AlarmOnDelay\" Type=\"xs:integer\" />\
<tt:SimpleItemDescription Name=\"AlarmOffDelay\" Type=\"xs:integer\" />\
<tt:SimpleItemDescription Type=\"xs:base64Binary\" Name=\"ActiveCells\">\
</tt:SimpleItemDescription>\
</tt:Parameters>\
<tt:Messages IsProperty=\"true\">\
<tt:Source>\
<tt:SimpleItemDescription Type=\"tt:ReferenceToken\" Name=\"VideoSource\">\
</tt:SimpleItemDescription>\
<tt:SimpleItemDescription Type=\"xs:string\" Name=\"RuleName\">\
</tt:SimpleItemDescription>\
</tt:Source><tt:Data>\
<tt:SimpleItemDescription Type=\"xs:boolean\" Name=\"State\">\
</tt:SimpleItemDescription>\
</tt:Data>\
<tt:ParentTopic>tns1:RuleEngine/CellMotionDetector/Motion</tt:ParentTopic>\
</tt:Messages>\
</tt:RuleDescription>\
</tan:SupportedRules>\
</tan:GetSupportedRulesResponse>\
</SOAP-ENV:Body>\
</SOAP-ENV:Envelope>\r\n";
*/
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Header></SOAP-ENV:Header>\n\t\
<SOAP-ENV:Body>\n\t\t\
<tan:GetSupportedRulesResponse>\
<tan:SupportedRules>\
<tt:RuleContentSchemaLocation>http://www.w3.org/2001/XMLSchema</tt:RuleContentSchemaLocation>\
<tt:RuleDescription Name=\"tt:MotionRegionDetector\">\
<tt:Parameters>\
<tt:ElementItemDescription Type=\"axt:MotionRegionConfig\" Name=\"MotionRegion\">\
</tt:ElementItemDescription></tt:Parameters>\
<tt:Messages IsProperty=\"true\">\
<tt:Source>\
<tt:SimpleItemDescription Type=\"tt:ReferenceToken\" Name=\"VideoSource\">\
</tt:SimpleItemDescription><tt:SimpleItemDescription Type=\"xs:string\" Name=\"RuleName\">\
</tt:SimpleItemDescription>\
</tt:Source>\
<tt:Data>\
<tt:SimpleItemDescription Type=\"xs:boolean\" Name=\"State\">\
</tt:SimpleItemDescription>\
</tt:Data>\
<tt:ParentTopic>tns1:RuleEngine/MotionRegionDetector/Motion</tt:ParentTopic>\
</tt:Messages>\
</tt:RuleDescription>\
</tan:SupportedRules>\
</tan:GetSupportedRulesResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n";

const char *ONVIF_HTTP_AddVideoEncoderConfiguration_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<trt:AddVideoEncoderConfigurationResponse>\n\t\t\
</trt:AddVideoEncoderConfigurationResponse>\n\t\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

const char *ONVIF_HTTP_GetGuaranteedNumberOfVideoEncoderInstances_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<trt:GetGuaranteedNumberOfVideoEncoderInstancesResponse>\n\t\t\
<trt:TotalNumber>2</trt:TotalNumber>\n\t\t\t\
<trt:JPEG>0</trt:JPEG>\n\t\t\t\
<trt:H264>2</trt:H264>\n\t\t\t\
<trt:MPEG4>0</trt:MPEG4>\n\t\t\t\
</trt:GetGuaranteedNumberOfVideoEncoderInstancesResponse>\n\t\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

const char *ONVIF_HTTP_GetVideoEncoderInstances_BODY =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<SOAP-ENV:Envelope %s\n\t\
<SOAP-ENV:Body>\n\t\t\
<tr2:GetVideoEncoderInstancesResponse>\n\t\t\t\
<tr2:Info>\n\t\t\t\t\
<tr2:Codec>\n\t\t\t\t\t\
<tr2:Encoding>H264</tr2:Encoding>\n\t\t\t\t\t\
<tr2:Number>2</tr2:Number>\n\t\t\t\t\t\
</tr2:Codec>\n\t\t\t\t\
<tr2:Codec>\n\t\t\t\t\t\
<tr2:Encoding>H265</tr2:Encoding>\n\t\t\t\t\t\
<tr2:Number>2</tr2:Number>\n\t\t\t\t\t\
</tr2:Codec>\n\t\t\t\t\
<tr2:Codec>\n\t\t\t\t\t\
<tr2:Encoding>JPEG</tr2:Encoding>\n\t\t\t\t\t\
<tr2:Number>0</tr2:Number>\n\t\t\t\t\t\
</tr2:Codec>\n\t\t\t\t\
<tr2:Total>2</tr2:Total>\n\t\t\t\
</tr2:Info>\n\t\t\
</tr2:GetVideoEncoderInstancesResponse>\n\t\
</SOAP-ENV:Body>\n</SOAP-ENV:Envelope>\r\n\
";

#endif
