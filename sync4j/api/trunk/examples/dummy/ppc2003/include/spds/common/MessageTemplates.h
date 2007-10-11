/*
 * Copyright (C) 2003-2005 Funambol
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// for clear auth 
// const wchar_t* xmlInit  = TEXT("<SyncML>\n<SyncHdr>\n<VerDTD>1.1</VerDTD>\n<VerProto>SyncML/1.1</VerProto>\n<SessionID>1</SessionID>\n<MsgID>1</MsgID>\n<Target><LocURI>%s</LocURI></Target>\n<Source><LocURI>%s</LocURI></Source>\n<Cred>\n<Meta><Type xmlns='syncml:metinf'>syncml:auth-clear</Type></Meta>\n<Data>%s:%s</Data>\n</Cred>\n</SyncHdr>\n<SyncBody>\n%s\n<Final/>\n</SyncBody>\n</SyncML>");

// for basic auth 
const wchar_t* xmlInitBasic  = TEXT("<SyncML>\n<SyncHdr>\n<VerDTD>1.1</VerDTD>\n<VerProto>SyncML/1.1</VerProto>\n<SessionID>1</SessionID>\n<MsgID>1</MsgID>\n<Target><LocURI>%s</LocURI></Target>\n<Source><LocURI>%s</LocURI></Source>\n<Cred>\n<Meta><Format xmlns='syncml:metinf'>b64</Format><Type xmlns='syncml:metinf'>syncml:auth-basic</Type></Meta>\n<Data>%s</Data>\n</Cred>\n</SyncHdr>\n<SyncBody>\n%s\n<Final/>\n</SyncBody>\n</SyncML>");
const wchar_t* xmlSync1 = TEXT("<SyncML><SyncHdr>\n<VerDTD>1.1</VerDTD>\n<VerProto>SyncML/1.1</VerProto>\n<SessionID>1</SessionID>\n<MsgID>2</MsgID>\n<Target><LocURI>%s</LocURI></Target>\n<Source><LocURI>%s</LocURI></Source>\n</SyncHdr>\n<SyncBody>\n<Status>\n<CmdID>1</CmdID>\n<MsgRef>1</MsgRef>\n<CmdRef>0</CmdRef>\n<Cmd>SyncHdr</Cmd>\n<TargetRef>%s</TargetRef>\n<SourceRef>%s</SourceRef>\n<Data>200</Data>\n</Status>\n<Status>\n<CmdID>2</CmdID>\n<MsgRef>1</MsgRef><CmdRef>1</CmdRef><Cmd>Alert</Cmd>\n<TargetRef>%s</TargetRef>\n<SourceRef>%s</SourceRef>\n<Data>200</Data>\n<Item>\n<Data>\n<Anchor xmlns='syncml:metinf'><Next>%s</Next></Anchor>\n</Data>\n</Item>\n</Status>\n<Sync><CmdID>3</CmdID>\n<Target><LocURI>%s</LocURI></Target>\n<Source><LocURI>%s</LocURI></Source>\n");
const wchar_t* xmlSync2 = TEXT("\n</Sync><Final/>\n</SyncBody>\n</SyncML>");
const wchar_t* xmlAlert = TEXT("<SyncML>\n<SyncHdr>\n<VerDTD>1.1</VerDTD><VerProto>SyncML/1.1</VerProto><SessionID>1</SessionID>\n<MsgID>3</MsgID>\n<Target>\n<LocURI>%s</LocURI>\n</Target>\n<Source><LocURI>%s</LocURI>\n</Source>\n</SyncHdr>\n<SyncBody>\n<Status>\n<CmdID>1</CmdID>\n<MsgRef>2</MsgRef>\n<CmdRef>0</CmdRef>\n<Cmd>SyncHdr</Cmd>\n<TargetRef>%s</TargetRef>\n<SourceRef>%s</SourceRef>\n<Data>200</Data>\n</Status>\n<Alert>\n<CmdID>2</CmdID>\n<Data>222</Data>\n<Item>\n<Target><LocURI>%s</LocURI></Target>\n<Source><LocURI>%s</LocURI></Source>\n</Item>\n</Alert>\n</SyncBody>\n</SyncML>");

const wchar_t* xmlMap   = TEXT("<SyncML>\n<SyncHdr>\n<VerDTD>1.1</VerDTD>\n<VerProto>SyncML/1.1</VerProto>\n<SessionID>1</SessionID>\n<MsgID>4</MsgID>\n<Target><LocURI>%s</LocURI></Target>\n<Source><LocURI>%s</LocURI></Source>\n</SyncHdr>\n<SyncBody>\n<Status>\n<CmdID>1</CmdID>\n<MsgRef>1</MsgRef>\n<CmdRef>0</CmdRef>\n<Cmd>SyncHdr</Cmd>\n<TargetRef>%s</TargetRef>\n<SourceRef>%s</SourceRef>\n<Data>200</Data>\n</Status>%s\n<Final/>\n</SyncBody>\n</SyncML>");

wchar_t* xmlMapStart1 = TEXT("<SyncML>\n<SyncHdr>\n<VerDTD>1.1</VerDTD>\n<VerProto>SyncML/1.1</VerProto>\n<SessionID>1</SessionID>\n<MsgID>4</MsgID>\n<Target><LocURI>%s</LocURI></Target>\n<Source><LocURI>%s</LocURI></Source>\n</SyncHdr>\n<SyncBody>\n<Status>\n<CmdID>1</CmdID>\n<MsgRef>1</MsgRef>\n<CmdRef>0</CmdRef>\n<Cmd>SyncHdr</Cmd>\n<TargetRef>%s</TargetRef>\n<SourceRef>%s</SourceRef>\n<Data>200</Data>\n</Status>");
wchar_t* xmlMapStart2 = TEXT("<Final/>\n</SyncBody>\n</SyncML>");
