/*
 * emergency module - basic support for emergency calls
 *
 * Copyright (C) 2014-2015 Robison Tesini & Evandro Villaron
 *
 * This file is part of opensips, a free SIP server.
 *
 * opensips is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * opensips is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * History:
 * --------
 *  2014-10-14 initial version (Villaron/Tesini)
 *  2013-03-21 implementing subscriber function (Villaron/Tesini)
 */


#include "../../sr_module.h"
#include "../../dprint.h"
#include "../../mem/mem.h"
#include "../../mem/shm_mem.h"
#include "../../mod_fix.h"
#include "../../socket_info.h"
#include "../../route_struct.h"
#include "../../ip_addr.h"
#include "../../parser/msg_parser.h"
#include "../../parser/parse_uri.h"
#include "../../parser/parse_pai.h"
#include "../../parser/parse_ppi.h"
#include "../../parser/parse_rpid.h"
#include "../../parser/parse_from.h"
#include "../../regexp.h"
#include "../../data_lump.h"
#include "../../data_lump_rpl.h" 
#include "../../ut.h"
#include "../../rw_locking.h"
#include "../../timer.h"
#include "../../db/db.h"
#include "../../db/db_insertq.h"
#include "../../forward.h"
#include "../rr/api.h"
#include "../tm/tm_load.h" /*load_tm_api*/

#include "http_emergency.h" 

 #define TIMER_B				30
 #define MAXNUMBERLEN 			31

struct sm_subscriber **subs_pt;
struct tm_binds eme_tm;

struct dlg_identity{
	str callid;
	str from_tag;
	str to_tag; 
};

struct sm_subscriber{
	struct dlg_identity dlg_id;
	int status;
	str loc_uri;
	str rem_uri;
	str contact;
	str callid_ori;
	str event;
	int expires;
	int timeout;
	struct sm_subscriber *prev;	
	struct sm_subscriber *next;
};

struct parms_cb{
	str callid_ori;
	str event;
};

int send_subscriber(struct sip_msg* msg, char* callidHeader, int expires);
int send_subscriber_within(struct sip_msg* msg, struct sm_subscriber* subs, int expires);
int get_uris_to_subscribe(struct sip_msg* msg, str* contact, str* notifier, str* subscriber );
int build_params_cb(struct sip_msg* msg, char* callidHeader,  struct parms_cb* params_cb );
str* add_hdr_subscriber(int expires, str event);
void subs_cback_func(struct cell *t, int cb_type, struct tmcb_params *params);
int extract_reply_headers(struct sip_msg* reply, str* callid, int expires);
int create_subscriber_cell(struct sip_msg* reply, struct parms_cb* params_cb);
int treat_notify(struct sip_msg *msg); 
struct sm_subscriber* get_subs_cell(struct sip_msg *msg);
void subs_cback_func_II(struct cell *t, int cb_type, struct tmcb_params *params);
dlg_t* build_dlg(struct sm_subscriber* subscriber);
struct sm_subscriber* find_subscriber_cell(str* callId, str* to_tag);
int same_dialog_id(struct sm_subscriber* subs_cell, str* callId, str* to_tag);
