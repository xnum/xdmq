#pragma once
#include "raft.h"

typedef struct
{
    int type;
    int stat;
    union
    {
        msg_requestvote_t rv;
        msg_requestvote_response_t rvr;
        msg_appendentries_t ae;
        msg_appendentries_response_t aer;
    };
    int reserved;
    int padding[100];
} msg_t;

enum msg_stat {
    NONE = 0,
    AE,
    AE_ENTRIES,
    AE_ENTRY_BODY,
    AER,
    RV,
    RVR,
    DONE
};

typedef enum
{
    /** Handshake is a special non-raft message type
     *      * We send a handshake so that we can identify ourselves to our peers */
    MSG_HANDSHAKE,
    /** Successful responses mean we can start the Raft periodic callback */
    MSG_HANDSHAKE_RESPONSE,
    /** Tell leader we want to leave the cluster */
    /* When instance is ctrl-c'd we have to gracefuly disconnect */
    MSG_LEAVE,
    /* Receiving a leave response means we can shutdown */
    MSG_LEAVE_RESPONSE,
    MSG_REQUESTVOTE,
    MSG_REQUESTVOTE_RESPONSE,
    MSG_APPENDENTRIES,
    MSG_APPENDENTRIES_RESPONSE,
} peer_message_type_e;

typedef struct
{
    int64_t node_id;
} msg_handshake_t;
