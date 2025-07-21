/**
 * @file net_config.h
 * @brief CycloneTCP configuration file
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2025 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneTCP Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.5.2
 **/

#ifndef _NET_CONFIG_H
#define _NET_CONFIG_H

//Dependencies
#include "RTE_Components.h"

//*** <<< Use Configuration Wizard in Context Menu >>> ***

// <o>Number of network adapters
// <i>Number of network adapters
// <i>Default: 1
// <1-16>
#define NET_INTERFACE_COUNT 1

// <h>Trace level

// <o>Memory Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define MEM_TRACE_LEVEL 0

// <o>NIC Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define NIC_TRACE_LEVEL 0

// <o>Ethernet Trace level
// <i>Set the desired debugging level
// <i>Default: Error
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define ETH_TRACE_LEVEL 0

// <o>LLDP Trace level
// <i>Set the desired debugging level
// <i>Default: Error
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define LLDP_TRACE_LEVEL 0

// <o>ARP Trace level
// <i>Set the desired debugging level
// <i>Default: Error
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define ARP_TRACE_LEVEL 0

// <o>IP Trace level
// <i>Set the desired debugging level
// <i>Default: Error
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define IP_TRACE_LEVEL 0

// <o>IPv4 Trace level
// <i>Set the desired debugging level
// <i>Default: Error
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define IPV4_TRACE_LEVEL 0

// <o>IPv6 Trace level
// <i>Set the desired debugging level
// <i>Default: Error
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define IPV6_TRACE_LEVEL 0

// <o>ICMP Trace level
// <i>Set the desired debugging level
// <i>Default: Error
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define ICMP_TRACE_LEVEL 0

// <o>IGMP Trace level
// <i>Set the desired debugging level
// <i>Default: Error
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define IGMP_TRACE_LEVEL 0

// <o>NAT Trace level
// <i>Set the desired debugging level
// <i>Default: Error
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define NAT_TRACE_LEVEL 0

// <o>ICMPv6 Trace level
// <i>Set the desired debugging level
// <i>Default: Error
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define ICMPV6_TRACE_LEVEL 0

// <o>MLD Trace level
// <i>Set the desired debugging level
// <i>Default: Error
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define MLD_TRACE_LEVEL 0

// <o>NDP Trace level
// <i>Set the desired debugging level
// <i>Default: Error
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define NDP_TRACE_LEVEL 0

// <o>PPP Trace level
// <i>Set the desired debugging level
// <i>Default: Error
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define PPP_TRACE_LEVEL 0

// <o>UDP Trace level
// <i>Set the desired debugging level
// <i>Default: Error
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define UDP_TRACE_LEVEL 0

// <o>TCP Trace level
// <i>Set the desired debugging level
// <i>Default: Error
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define TCP_TRACE_LEVEL 0

// <o>Socket Trace level
// <i>Set the desired debugging level
// <i>Default: Error
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define SOCKET_TRACE_LEVEL 0

// <o>Raw socket Trace level
// <i>Set the desired debugging level
// <i>Default: Error
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define RAW_SOCKET_TRACE_LEVEL 0

// <o>BSD socket Trace level
// <i>Set the desired debugging level
// <i>Default: Error
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define BSD_SOCKET_TRACE_LEVEL 0

// <o>WebSocket Trace level
// <i>Set the desired debugging level
// <i>Default: Error
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define WEB_SOCKET_TRACE_LEVEL 0

// <o>Auto-IP Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define AUTO_IP_TRACE_LEVEL 0

// <o>SLAAC Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define SLAAC_TRACE_LEVEL 0

// <o>DHCP Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define DHCP_TRACE_LEVEL 0

// <o>DHCPv6 Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define DHCPV6_TRACE_LEVEL 0

// <o>DNS Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define DNS_TRACE_LEVEL 0

// <o>mDNS Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define MDNS_TRACE_LEVEL 0

// <o>NBNS Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define NBNS_TRACE_LEVEL 0

// <o>LLMNR Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define LLMNR_TRACE_LEVEL 0

// <o>CoAP Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define COAP_TRACE_LEVEL 0

// <o>FTP Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define FTP_TRACE_LEVEL 0

// <o>HTTP Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define HTTP_TRACE_LEVEL 0

// <o>MQTT Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define MQTT_TRACE_LEVEL 0

// <o>MQTT-SN Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define MQTT_SN_TRACE_LEVEL 0

// <o>SMTP Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define SMTP_TRACE_LEVEL 0

// <o>SNMP Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define SNMP_TRACE_LEVEL 0

// <o>SNTP Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define SNTP_TRACE_LEVEL 0

// <o>NTP Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define NTP_TRACE_LEVEL 0

// <o>NTS Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define NTS_TRACE_LEVEL 0

// <o>TFTP Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define TFTP_TRACE_LEVEL 0

// <o>Modbus/TCP Trace level
// <i>Set the desired debugging level
// <i>Default: Info
// <0=>Off
// <1=>Fatal
// <2=>Error
// <3=>Warning
// <4=>Info
// <5=>Debug
// <6=>Verbose
#define MODBUS_TRACE_LEVEL 0

// </h>
// <h>Ethernet

// <q>Virtual interface support
// <i>Enable support for virtual interfaces
// <i>Default: Disabled
#define ETH_VIRTUAL_IF_SUPPORT 0

// <q>VLAN support
// <i>Enable VLAN support (IEEE 802.1q)
// <i>Default: Disabled
#define ETH_VLAN_SUPPORT 0

// <q>LLC support
// <i>Enable LLC (IEEE 802.2)
// <i>Default: Disabled
#define ETH_LLC_SUPPORT 0

// <q>Switch port tagging support
// <i>Enable switch port tagging
// <i>Default: Disabled
#define ETH_PORT_TAGGING_SUPPORT 0

// <o>Size of the multicast MAC filter
// <i>Maximum number of entries in the multicast MAC filter
// <i>Default: 12
// <1-100>
#define MAC_MULTICAST_FILTER_SIZE 12

// </h>
// <h>LLDP

// <q>TX mode
// <i>Enable transmission of LLDP frames
// <i>Default: Enabled
#define LLDP_TX_MODE_SUPPORT 1

// <q>RX mode
// <i>Enable reception of LLDP frames
// <i>Default: Disabled
#define LLDP_RX_MODE_SUPPORT 0

// <o>Maximum LLDPDU size
// <i>Maximum length of LLDP data units
// <i>Default: 500
// <100-1500>
#define LLDP_MAX_LLDPDU_SIZE 500

// <o>Maximum number of management addresses
// <i>Set the maximum number of management addresses
// <i>Default: 10
// <1-32>
#define LLDP_MAX_MGMT_ADDRS 10

// </h>
// <h>IPv4

// <o>Maximum number of IPv4 addresses
// <i>Set the maximum number of IPv4 addresses
// <i>Default: 1
// <1-10>
#define IPV4_ADDR_LIST_SIZE 1

// <o>Size of the IPv4 multicast filter
// <i>Maximum number of entries in the IPv4 multicast filter
// <i>Default: 4
// <1-100>
#define IPV4_MULTICAST_FILTER_SIZE 4

// <q>IPv4 fragmentation support
// <i>Enable IPv4 fragmentation and reassembly support
// <i>Default: Enabled
#define IPV4_FRAG_SUPPORT 1

// <o>Maximum number of fragmented packets
// <i>Maximum number of fragmented packets the host will accept and hold in the reassembly queue simultaneously
// <i>Default: 4
// <1-100>
#define IPV4_MAX_FRAG_DATAGRAMS 4

// <o>Maximum datagram size
// <i>Maximum datagram size the host will accept when reassembling fragments
// <i>Default: 8192
// <576-65536>
#define IPV4_MAX_FRAG_DATAGRAM_SIZE 8192

// <o>Size of ARP cache
// <i>Size of ARP cache
// <i>Default: 8
// <1-100>
#define ARP_CACHE_SIZE 8

// <o>Maximum number of pending packets
// <i>Maximum number of packets waiting for address resolution to complete
// <i>Default: 2
// <1-100>
#define ARP_MAX_PENDING_PACKETS 2

// </h>
// <h>IPv6

// <o>Maximum number of IPv6 unicast addresses
// <i>Set the maximum number of IPv6 unicast addresses
// <i>Default: 3
// <1-10>
#define IPV6_ADDR_LIST_SIZE 3

// <o>Maximum number of IPv6 anycast addresses
// <i>Set the maximum number of IPv6 anycast addresses
// <i>Default: 1
// <1-10>
#define IPV6_ANYCAST_ADDR_LIST_SIZE 1

// <o>Size of the prefix list
// <i>Maximum number of entries in the prefix list
// <i>Default: 2
// <1-10>
#define IPV6_PREFIX_LIST_SIZE 2

// <o>Size of the router list
// <i>Maximum number number of default routers
// <i>Default: 2
// <1-10>
#define IPV6_ROUTER_LIST_SIZE 2

// <o>Size of the IPv6 multicast filter
// <i>Maximum number of entries in the IPv6 multicast filter
// <i>Default: 8
// <1-100>
#define IPV6_MULTICAST_FILTER_SIZE 8

// <q>IPv6 fragmentation support
// <i>Enable IPv6 fragmentation and reassembly support
// <i>Default: Enabled
#define IPV6_FRAG_SUPPORT 1

// <o>Maximum number of fragmented packets
// <i>Maximum number of fragmented packets the host will accept and hold in the reassembly queue simultaneously
// <i>Default: 4
// <1-100>
#define IPV6_MAX_FRAG_DATAGRAMS 4

// <o>Maximum datagram size
// <i>Maximum datagram size the host will accept when reassembling fragments
// <i>Default: 8192
// <576-65536>
#define IPV6_MAX_FRAG_DATAGRAM_SIZE 8192

// <o>Size of the Neighbor Cache size
// <i>Size of the Neighbor Cache size
// <i>Default: 8
// <1-100>
#define NDP_NEIGHBOR_CACHE_SIZE 8

// <o>Size of the Destination Cache size
// <i>Size of the Destination Cache size
// <i>Default: 8
// <1-100>
#define NDP_DEST_CACHE_SIZE 8

// <o>Maximum number of pending packets
// <i>Maximum number of packets waiting for address resolution to complete
// <i>Default: 2
// <1-100>
#define NDP_MAX_PENDING_PACKETS 2

// </h>
// <h>TCP

// <o>Default buffer size for transmission
// <i>Default buffer size for transmission
// <i>Default: 2860
// <576-65536>
#define TCP_DEFAULT_TX_BUFFER_SIZE 2860

// <o>Default buffer size for reception
// <i>Default buffer size for reception
// <i>Default: 2860
// <576-65536>
#define TCP_DEFAULT_RX_BUFFER_SIZE 2860

// <o>Default SYN queue size for listening sockets
// <i>Default SYN queue size for listening sockets
// <i>Default: 4
// <1-100>
#define TCP_DEFAULT_SYN_QUEUE_SIZE 4

// <o>Maximum number of retransmissions
// <i>Maximum number of retransmissions
// <i>Default: 5
// <1-100>
#define TCP_MAX_RETRIES 5

// <q>SACK support
// <i>Enable selective acknowledgment support
// <i>Default: Disabled
#define TCP_SACK_SUPPORT 0

// <q>TCP keep-alive support
// <i>Enable TCP keep-alive support
// <i>Default: Disabled
#define TCP_KEEP_ALIVE_SUPPORT 0

// </h>
// <h>UDP

// <o>Receive queue depth for connectionless sockets
// <i>Receive queue depth for connectionless sockets
// <i>Default: 4
// <1-100>
#define UDP_RX_QUEUE_SIZE 4

// </h>
// <h>Socket

// <o>Maximum number of sockets
// <i>Maximum number of sockets
// <i>Default: 16
// <1-100>
//Number of sockets that can be opened simultaneously
#define SOCKET_MAX_COUNT 16

// <q>BSD socket support
// <i>Enable BSD socket support
// <i>Default: Disabled
#define BSD_SOCKET_SUPPORT 0

// <q>Raw socket support
// <i>Enable raw socket support
// <i>Default: Disabled
#define RAW_SOCKET_SUPPORT 0

// </h>
// <h>HTTP Server

// <q>File system support
// <i>Enable file system support
// <i>Default: Disabled
#define HTTP_SERVER_FS_SUPPORT 0

// <q>SSI support
// <i>Enable Server Side Includes support
// <i>Default: Disabled
#define HTTP_SERVER_SSI_SUPPORT 0

// <q>Basic access authentication support
// <i>Enable basic access authentication support
// <i>Default: Disabled
#define HTTP_SERVER_BASIC_AUTH_SUPPORT 0

// <q>Digest access authentication support
// <i>Enable digest access authentication support
// <i>Default: Disabled
#define HTTP_SERVER_DIGEST_AUTH_SUPPORT 0

// <q>WebSocket support
// <i>Enable WebSocket support
// <i>Default: Disabled
#define HTTP_SERVER_WEB_SOCKET_SUPPORT 0

// <o>HTTP connection timeout
// <i>HTTP connection timeout (in seconds)
// <i>Default: 10
// <1-600>
// <#*1000>
#define HTTP_SERVER_TIMEOUT 10000

// <o>HTTP idle timeout
// <i>Maximum time the server will wait for a subsequent request before closing the connection (in seconds)
// <i>Default: 5
// <1-600>
// <#*1000>
#define HTTP_SERVER_IDLE_TIMEOUT 5000

// </h>
// <h>SNMP Agent

// <q>SNMPv1 support
// <i>Enable SNMPv1 support
// <i>Default: Enabled
#define SNMP_V1_SUPPORT 1

// <q>SNMPv2c support
// <i>Enable SNMPv2c support
// <i>Default: Enabled
#define SNMP_V2C_SUPPORT 1

// <q>SNMPv3 support
// <i>Enable SNMPv3 support
// <i>Default: Disabled
#define SNMP_V3_SUPPORT 0

// <o>Maximum size of SNMP messages
// <i>Maximum size of SNMP messages
// <i>Default: 1452
// <484-65535>
#define SNMP_MAX_MSG_SIZE 1452

// <o>Maximum number of MIBs
// <i>Maximum number of MIBs that can be loaded at a time
// <i>Default: 8
// <1-100>
#define SNMP_AGENT_MAX_MIBS 8

// <o>Maximum number of community strings
// <i>Maximum number of community strings
// <i>Default: 3
// <1-100>
#define SNMP_AGENT_MAX_COMMUNITIES 3

// <o>Maximum number of users
// <i>Maximum number of users
// <i>Default: 8
// <1-100>
#define SNMP_AGENT_MAX_USERS 8

// <q>VACM support
// <i>Enable VACM support
// <i>Default: Disabled
#define SNMP_AGENT_VACM_SUPPORT 0

// <o>Size of the group table
// <i>Size of the group table
// <i>Default: 8
// <1-100>
#define SNMP_AGENT_GROUP_TABLE_SIZE 8

// <o>Size of the access table
// <i>Size of the access table
// <i>Default: 8
// <1-100>
#define SNMP_AGENT_ACCESS_TABLE_SIZE 8

// <o>Size of the view table
// <i>Size of the view table
// <i>Default: 8
// <1-100>
#define SNMP_AGENT_VIEW_TABLE_SIZE 8

// </h>

//LLDP agent support
#ifdef RTE_CYCLONE_TCP_LLDP
   #define LLDP_SUPPORT ENABLED
#else
   #define LLDP_SUPPORT DISABLED
#endif

//IPv4 support
#ifdef RTE_CYCLONE_TCP_IPV4
   #define IPV4_SUPPORT ENABLED
#else
   #define IPV4_SUPPORT DISABLED
#endif

//IPsec support
#ifdef RTE_CYCLONE_IPSEC_IPSEC
   #define IPV4_IPSEC_SUPPORT ENABLED
#else
   #define IPV4_IPSEC_SUPPORT DISABLED
#endif

//Auto-IP support
#ifdef RTE_CYCLONE_TCP_AUTO_IP
   #define AUTO_IP_SUPPORT ENABLED
#else
   #define AUTO_IP_SUPPORT DISABLED
#endif

//IGMP host support
#ifdef RTE_CYCLONE_TCP_IGMP_HOST
   #define IGMP_HOST_SUPPORT ENABLED
#else
   #define IGMP_HOST_SUPPORT DISABLED
#endif

//IGMP router support
#ifdef RTE_CYCLONE_TCP_IGMP_ROUTER
   #define IGMP_ROUTER_SUPPORT ENABLED
#else
   #define IGMP_ROUTER_SUPPORT DISABLED
#endif

//IGMP snooping support
#ifdef RTE_CYCLONE_TCP_IGMP_SNOOPING
   #define IGMP_SNOOPING_SUPPORT ENABLED
#else
   #define IGMP_SNOOPING_SUPPORT DISABLED
#endif

//NAT support
#ifdef RTE_CYCLONE_TCP_NAT
   #define NAT_SUPPORT ENABLED
#else
   #define NAT_SUPPORT DISABLED
#endif

//IPv6 support
#ifdef RTE_CYCLONE_TCP_IPV6
   #define IPV6_SUPPORT ENABLED
#else
   #define IPV6_SUPPORT DISABLED
#endif

//MLD node support
#ifdef RTE_CYCLONE_TCP_MLD_NODE
   #define MLD_NODE_SUPPORT ENABLED
#else
   #define MLD_NODE_SUPPORT DISABLED
#endif

//SLAAC support
#ifdef RTE_CYCLONE_TCP_SLAAC
   #define SLAAC_SUPPORT ENABLED
#else
   #define SLAAC_SUPPORT DISABLED
#endif

//RA service support
#ifdef RTE_CYCLONE_TCP_ROUTER_ADV
   #define NDP_ROUTER_ADV_SUPPORT ENABLED
#else
   #define NDP_ROUTER_ADV_SUPPORT DISABLED
#endif

//TCP support
#ifdef RTE_CYCLONE_TCP_TCP
   #define TCP_SUPPORT ENABLED
#else
   #define TCP_SUPPORT DISABLED
#endif

//UDP support
#ifdef RTE_CYCLONE_TCP_UDP
   #define UDP_SUPPORT ENABLED
#else
   #define UDP_SUPPORT DISABLED
#endif

//DHCP client support
#ifdef RTE_CYCLONE_TCP_DHCP_CLIENT
   #define DHCP_CLIENT_SUPPORT ENABLED
#else
   #define DHCP_CLIENT_SUPPORT DISABLED
#endif

//DHCP server support
#ifdef RTE_CYCLONE_TCP_DHCP_SERVER
   #define DHCP_SERVER_SUPPORT ENABLED
#else
   #define DHCP_SERVER_SUPPORT DISABLED
#endif

//DHCPv6 client support
#ifdef RTE_CYCLONE_TCP_DHCPV6_CLIENT
   #define DHCPV6_CLIENT_SUPPORT ENABLED
#else
   #define DHCPV6_CLIENT_SUPPORT DISABLED
#endif

//DHCPv6 relay agent support
#ifdef RTE_CYCLONE_TCP_DHCPV6_RELAY
   #define DHCPV6_RELAY_SUPPORT ENABLED
#else
   #define DHCPV6_RELAY_SUPPORT DISABLED
#endif

//DNS client support
#ifdef RTE_CYCLONE_TCP_DNS_CLIENT
   #define DNS_CLIENT_SUPPORT ENABLED
#else
   #define DNS_CLIENT_SUPPORT DISABLED
#endif

//mDNS client support
#ifdef RTE_CYCLONE_TCP_MDNS_CLIENT
   #define MDNS_CLIENT_SUPPORT ENABLED
#else
   #define MDNS_CLIENT_SUPPORT DISABLED
#endif

//mDNS responder support
#ifdef RTE_CYCLONE_TCP_MDNS_RESPONDER
   #define MDNS_RESPONDER_SUPPORT ENABLED
#else
   #define MDNS_RESPONDER_SUPPORT DISABLED
#endif

//DNS-SD responder support
#ifdef RTE_CYCLONE_TCP_DNS_SD_RESPONDER
   #define DNS_SD_RESPONDER_SUPPORT ENABLED
#else
   #define DNS_SD_RESPONDER_SUPPORT DISABLED
#endif

//NBNS client support
#ifdef RTE_CYCLONE_TCP_NBNS_CLIENT
   #define NBNS_CLIENT_SUPPORT ENABLED
#else
   #define NBNS_CLIENT_SUPPORT DISABLED
#endif

//NBNS responder support
#ifdef RTE_CYCLONE_TCP_NBNS_RESPONDER
   #define NBNS_RESPONDER_SUPPORT ENABLED
#else
   #define NBNS_RESPONDER_SUPPORT DISABLED
#endif

//LLMNR client support
#ifdef RTE_CYCLONE_TCP_LLMNR_CLIENT
   #define LLMNR_CLIENT_SUPPORT ENABLED
#else
   #define LLMNR_CLIENT_SUPPORT DISABLED
#endif

//LLMNR responder support
#ifdef RTE_CYCLONE_TCP_LLMNR_RESPONDER
   #define LLMNR_RESPONDER_SUPPORT ENABLED
#else
   #define LLMNR_RESPONDER_SUPPORT DISABLED
#endif

//CoAP client support
#ifdef RTE_CYCLONE_TCP_COAP_CLIENT
   #define COAP_CLIENT_SUPPORT ENABLED
#else
   #define COAP_CLIENT_SUPPORT DISABLED
#endif

//CoAP over DTLS
#ifdef RTE_CYCLONE_TCP_COAP_CLIENT_DTLS
   #define COAP_CLIENT_DTLS_SUPPORT ENABLED
#else
   #define COAP_CLIENT_DTLS_SUPPORT DISABLED
#endif

//CoAP server support
#ifdef RTE_CYCLONE_TCP_COAP_SERVER
   #define COAP_SERVER_SUPPORT ENABLED
#else
   #define COAP_SERVER_DTLS_SUPPORT DISABLED
#endif

//CoAP over DTLS
#ifdef RTE_CYCLONE_TCP_COAP_SERVER_DTLS
   #define COAP_SERVER_DTLS_SUPPORT ENABLED
#else
   #define COAP_SERVER_DTLS_SUPPORT DISABLED
#endif

//FTP client support
#ifdef RTE_CYCLONE_TCP_FTP_CLIENT
   #define FTP_CLIENT_SUPPORT ENABLED
#else
   #define FTP_CLIENT_SUPPORT DISABLED
#endif

//FTP over TLS
#ifdef RTE_CYCLONE_TCP_FTP_CLIENT_TLS
   #define FTP_CLIENT_TLS_SUPPORT ENABLED
#else
   #define FTP_CLIENT_TLS_SUPPORT DISABLED
#endif

//FTP server support
#ifdef RTE_CYCLONE_TCP_FTP_SERVER
   #define FTP_SERVER_SUPPORT ENABLED
#else
   #define FTP_SERVER_SUPPORT DISABLED
#endif

//FTP over TLS
#ifdef RTE_CYCLONE_TCP_FTP_SERVER_TLS
   #define FTP_SERVER_TLS_SUPPORT ENABLED
#else
   #define FTP_SERVER_TLS_SUPPORT DISABLED
#endif

//HTTP client support
#ifdef RTE_CYCLONE_TCP_HTTP_CLIENT
   #define HTTP_CLIENT_SUPPORT ENABLED
#else
   #define HTTP_CLIENT_SUPPORT DISABLED
#endif

//HTTP over TLS
#ifdef RTE_CYCLONE_TCP_HTTP_CLIENT_TLS
   #define HTTP_CLIENT_TLS_SUPPORT ENABLED
#else
   #define HTTP_CLIENT_TLS_SUPPORT DISABLED
#endif

//HTTP server support
#ifdef RTE_CYCLONE_TCP_HTTP_SERVER
   #define HTTP_SERVER_SUPPORT ENABLED
#else
   #define HTTP_SERVER_SUPPORT DISABLED
#endif

//HTTP over TLS
#ifdef RTE_CYCLONE_TCP_HTTP_SERVER_TLS
   #define HTTP_SERVER_TLS_SUPPORT ENABLED
#else
   #define HTTP_SERVER_TLS_SUPPORT DISABLED
#endif

//MQTT client support
#ifdef RTE_CYCLONE_TCP_MQTT_CLIENT
   #define MQTT_CLIENT_SUPPORT ENABLED
#else
   #define MQTT_CLIENT_SUPPORT DISABLED
#endif

//MQTT over TLS
#ifdef RTE_CYCLONE_TCP_MQTT_CLIENT_TLS
   #define MQTT_CLIENT_TLS_SUPPORT ENABLED
#else
   #define MQTT_CLIENT_TLS_SUPPORT DISABLED
#endif

//MQTT over WebSocket
#ifdef RTE_CYCLONE_TCP_MQTT_CLIENT_WS
   #define MQTT_CLIENT_WS_SUPPORT ENABLED
#else
   #define MQTT_CLIENT_WS_SUPPORT DISABLED
#endif

//MQTT-SN client support
#ifdef RTE_CYCLONE_TCP_MQTT_SN_CLIENT
   #define MQTT_CLIENT_SN_SUPPORT ENABLED
#else
   #define MQTT_CLIENT_SN_SUPPORT DISABLED
#endif

//MQTT-SN over DTLS
#ifdef RTE_CYCLONE_TCP_MQTT_SN_CLIENT_DTLS
   #define MQTT_SN_CLIENT_DTLS_SUPPORT ENABLED
#else
   #define MQTT_SN_CLIENT_DTLS_SUPPORT DISABLED
#endif

//SMTP client support
#ifdef RTE_CYCLONE_TCP_SMTP_CLIENT
   #define SMTP_CLIENT_SUPPORT ENABLED
#else
   #define SMTP_CLIENT_SUPPORT DISABLED
#endif

//SMTP over TLS
#ifdef RTE_CYCLONE_TCP_SMTP_CLIENT_TLS
   #define SMTP_CLIENT_TLS_SUPPORT ENABLED
#else
   #define SMTP_CLIENT_TLS_SUPPORT DISABLED
#endif

//SNMP agent support
#ifdef RTE_CYCLONE_TCP_SNMP_AGENT
   #define SNMP_AGENT_SUPPORT ENABLED
#else
   #define SNMP_AGENT_SUPPORT DISABLED
#endif

//MIB-II module support
#ifdef RTE_CYCLONE_TCP_MIB2
   #define MIB2_SUPPORT ENABLED
#else
   #define MIB2_SUPPORT DISABLED
#endif

//IF-MIB module support
#ifdef RTE_CYCLONE_TCP_IF_MIB
   #define IF_MIB_SUPPORT ENABLED
#else
   #define IF_MIB_SUPPORT DISABLED
#endif

//IP-MIB module support
#ifdef RTE_CYCLONE_TCP_IP_MIB
   #define IP_MIB_SUPPORT ENABLED
#else
   #define IP_MIB_SUPPORT DISABLED
#endif

//TCP-MIB module support
#ifdef RTE_CYCLONE_TCP_TCP_MIB
   #define TCP_MIB_SUPPORT ENABLED
#else
   #define TCP_MIB_SUPPORT DISABLED
#endif

//UDP-MIB module support
#ifdef RTE_CYCLONE_TCP_UDP_MIB
   #define UDP_MIB_SUPPORT ENABLED
#else
   #define UDP_MIB_SUPPORT DISABLED
#endif

//SNMP-MIB module support
#ifdef RTE_CYCLONE_TCP_SNMP_MIB
   #define SNMP_MIB_SUPPORT ENABLED
#else
   #define SNMP_MIB_SUPPORT DISABLED
#endif

//SNMP-FRAMEWORK-MIB module support
#ifdef RTE_CYCLONE_TCP_SNMP_FRAMEWORK_MIB
   #define SNMP_FRAMEWORK_MIB_SUPPORT ENABLED
#else
   #define SNMP_FRAMEWORK_MIB_SUPPORT DISABLED
#endif

//SNMP-COMMUNITY-MIB module support
#ifdef RTE_CYCLONE_TCP_SNMP_COMMUNITY_MIB
   #define SNMP_COMMUNITY_MIB_SUPPORT ENABLED
#else
   #define SNMP_COMMUNITY_MIB_SUPPORT DISABLED
#endif

//SNMP-USM-MIB module support
#ifdef RTE_CYCLONE_TCP_SNMP_USM_MIB
   #define SNMP_USM_MIB_SUPPORT ENABLED
#else
   #define SNMP_USM_MIB_SUPPORT DISABLED
#endif

//SNMP-VACM-MIB module support
#ifdef RTE_CYCLONE_TCP_SNMP_VACM_MIB
   #define SNMP_VACM_MIB_SUPPORT ENABLED
#else
   #define SNMP_VACM_MIB_SUPPORT DISABLED
#endif

//LLDP-MIB module support
#ifdef RTE_CYCLONE_TCP_LLDP_MIB
   #define LLDP_MIB_SUPPORT ENABLED
#else
   #define LLDP_MIB_SUPPORT DISABLED
#endif

//SNTP client support
#ifdef RTE_CYCLONE_TCP_SNTP_CLIENT
   #define SNTP_CLIENT_SUPPORT ENABLED
#else
   #define SNTP_CLIENT_SUPPORT DISABLED
#endif

//NTS client support
#ifdef RTE_CYCLONE_TCP_NTS_CLIENT
   #define NTS_CLIENT_SUPPORT ENABLED
#else
   #define NTS_CLIENT_SUPPORT DISABLED
#endif

//TFTP client support
#ifdef RTE_CYCLONE_TCP_TFTP_CLIENT
   #define TFTP_CLIENT_SUPPORT ENABLED
#else
   #define TFTP_CLIENT_SUPPORT DISABLED
#endif

//TFTP server support
#ifdef RTE_CYCLONE_TCP_TFTP_SERVER
   #define TFTP_SERVER_SUPPORT ENABLED
#else
   #define TFTP_SERVER_SUPPORT DISABLED
#endif

//Icecast client support
#ifdef RTE_CYCLONE_TCP_ICECAST_CLIENT
   #define ICECAST_CLIENT_SUPPORT ENABLED
#else
   #define ICECAST_CLIENT_SUPPORT DISABLED
#endif

//Modbus/TCP client support
#ifdef RTE_CYCLONE_TCP_MODBUS_CLIENT
   #define MODBUS_CLIENT_SUPPORT ENABLED
#else
   #define MODBUS_CLIENT_SUPPORT DISABLED
#endif

//Modbus/TCP over TLS
#ifdef RTE_CYCLONE_TCP_MODBUS_CLIENT_TLS
   #define MODBUS_CLIENT_TLS_SUPPORT ENABLED
#else
   #define MODBUS_CLIENT_TLS_SUPPORT DISABLED
#endif

//Modbus/TCP server support
#ifdef RTE_CYCLONE_TCP_MODBUS_SERVER
   #define MODBUS_SERVER_SUPPORT ENABLED
#else
   #define MODBUS_SERVER_SUPPORT DISABLED
#endif

//Modbus/TCP over TLS
#ifdef RTE_CYCLONE_TCP_MODBUS_SERVER_TLS
   #define MODBUS_SERVER_TLS_SUPPORT ENABLED
#else
   #define MODBUS_SERVER_TLS_SUPPORT DISABLED
#endif

//Syslog client support
#ifdef RTE_CYCLONE_TCP_SYSLOG_CLIENT
   #define SYSLOG_CLIENT_SUPPORT ENABLED
#else
   #define SYSLOG_CLIENT_SUPPORT DISABLED
#endif

//WebSocket support
#ifdef RTE_CYCLONE_TCP_WEB_SOCKET
   #define WEB_SOCKET_SUPPORT ENABLED
#else
   #define WEB_SOCKET_SUPPORT DISABLED
#endif

//Support for WebSocket connections over TLS
#ifdef RTE_CYCLONE_TCP_WEB_SOCKET_TLS
   #define WEB_SOCKET_TLS_SUPPORT ENABLED
#else
   #define WEB_SOCKET_TLS_SUPPORT DISABLED
#endif

//Ping utility support
#ifdef RTE_CYCLONE_TCP_PING
   #define PING_SUPPORT ENABLED
#else
   #define PING_SUPPORT DISABLED
#endif

//PPP support
#ifdef RTE_CYCLONE_TCP_PPP
   #define PPP_SUPPORT ENABLED
#else
   #define PPP_SUPPORT DISABLED
#endif

//PAP authentication support
#ifdef RTE_CYCLONE_TCP_PAP
   #define PAP_SUPPORT ENABLED
#else
   #define PAP_SUPPORT DISABLED
#endif

//CHAP authentication support
#ifdef RTE_CYCLONE_TCP_CHAP
   #define CHAP_SUPPORT ENABLED
#else
   #define CHAP_SUPPORT DISABLED
#endif

#endif
