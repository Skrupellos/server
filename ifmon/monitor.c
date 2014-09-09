#include <stdio.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <net/if.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include "consumer.h"
#include "monitor.h"
#include "util.h"

#define MAX_MSG_SIZE 4096


// For now, this is sufficient, but one may add more feature from chromium
// http://src.chromium.org/svn/trunk/src/net/base/address_tracker_linux.cc

int parseNlMsg(struct nlmsghdr *nh);
void parseRouteMsg(const struct rtattr* attr, struct ifaddrmsg* ifa, void** addr);



int
monitorRun(void) {
	int              rc;
	int              sock;
	char             buf[MAX_MSG_SIZE];
	int              len;
	struct nlmsghdr* nh;
	
	static const struct sockaddr_nl addr = {
		.nl_family = AF_NETLINK,
		.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR
	};
	
	
	// Create a socket
	if((sock = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0) {
		RET_ERRNO("Can't open socket");
	}
	
	// Bind to that socket
	if((rc = bind(sock, (struct sockaddr *)&addr, sizeof(addr))) < 0) {
		RET_ERRNO("Can't bind socket to netlink");
	}
	
	// Main loop: receive messages
	while((len = recv(sock, buf, sizeof(buf), 0)) > 0) {
		for(
			nh = (struct nlmsghdr *) buf;
			NLMSG_OK(nh, (unsigned int)len) && nh->nlmsg_type != NLMSG_DONE;
			nh = NLMSG_NEXT(nh, len)             // Automatically decrements len
		) {
			if(nh->nlmsg_type == NLMSG_ERROR) {
				RET_ERR("Received error message. This should never happen");
			}
			
			parseNlMsg(nh); // Ignore the returned errors
		}
		
		consumerStop();
	}
	
	// Check if main loop has been left gracefully
	if(len == 0) {
		printf("Peer closed netlink connection.\n");
		return EXIT_SUCCESS;
	}
	else {
		RET_ERRNO("Can't receive next message");
	}
}



int
parseNlMsg(struct nlmsghdr *nh) {
	struct ifaddrmsg* ifa;
	struct ConsumerMessage msg;
	size_t len;
	void * addr = NULL;
	
	switch(nh->nlmsg_type) {
		case RTM_NEWADDR:
		case RTM_DELADDR:
			ifa = (struct ifaddrmsg *)NLMSG_DATA(nh);
			
			len = IFA_PAYLOAD(nh);
			for(
				const struct rtattr * attr = IFA_RTA(ifa);
				RTA_OK(attr, (unsigned int)len);
				attr = RTA_NEXT(attr, len)       // Automatically decrements len
			) {
				parseRouteMsg(attr, ifa, &addr);
			}
			
			// Set the IP
			if(addr != NULL) {
				inet_ntop(ifa->ifa_family, addr, msg.ip, sizeof(msg.ip));
			}
			else {
				msg.ip[0] = '\0';
			}
			
			// Set interface
			if(if_indextoname(ifa->ifa_index, msg.name) == NULL) {
				RET_ERRNO("Can't get name of interface %i", ifa->ifa_index);
			}
			
			// Set the version
			msg.version = ifa->ifa_family == AF_INET ? 4 : 6;
			
			// Set removed
			msg.removed = nh->nlmsg_type == RTM_DELADDR;
			
			// Send message
			consumerConsume(&msg);
			
			break;
		default:
			printf("Unknown message type %i\n", nh->nlmsg_type);
			fflush(stdout);
	}
	
	return EXIT_SUCCESS;
}

void
parseRouteMsg(const struct rtattr* attr, struct ifaddrmsg* ifa, void** addr) {
	switch (attr->rta_type) {
		// http://lxr.free-electrons.com/source/net/ipv4/devinet.c#L1505
		// http://lxr.free-electrons.com/source/net/ipv6/addrconf.c#L3984
		case IFA_ADDRESS:
			// Prefer local addresses
			if(*addr != NULL) break;
		case IFA_LOCAL:
			if(
				ifa->ifa_family == AF_INET &&
				RTA_PAYLOAD(attr) < sizeof(struct in_addr)
			) {
				PRINT_ERR("IPv4 address is to short (%i)", RTA_PAYLOAD(attr));
			}
			else if(
				ifa->ifa_family == AF_INET6 &&
				RTA_PAYLOAD(attr) < sizeof(struct in6_addr)
			) {
				PRINT_ERR("IPv6 address is to short (%i)", RTA_PAYLOAD(attr));
			}
			else {
				*addr = RTA_DATA(attr);
			}
			
			break;
		
		default:
			break;
	}
}
