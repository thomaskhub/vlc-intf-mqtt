/*****************************************************************************
 * hotkeys.c: Hotkey handling for vlc
 *****************************************************************************
 * Copyright (C) 2022 Nobody
 * 
 *
 * Authors: someone 
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define VLC_MODULE_LICENSE VLC_LICENSE_GPL_2_PLUS
#include <vlc_common.h>

// #include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
#include <vlc_variables.h>
#include <vlc_vout_osd.h>
#include <vlc_vout.h>
// #include <vlc_playlist.h>

#define DisplayIcon(vout, icon) \
    do { if(vout) vout_OSDIcon(vout, VOUT_SPU_CHANNEL_OSD, icon); } while(0)



#define MODULE_STRING "mqtt"

// #include <vlc_playlist.h>
#include <vlc_actions.h>
#include <MQTTAsync.h>
#include <MQTTClient.h>
// #include <jansson.h>


#include <assert.h>

#define MQTT_URL "tcp://127.0.0.1:1883"
#define MQTT_CLIENT_ID "vlc"

/*****************************************************************************
 * intf_sys_t: description and status of FB interface
 *****************************************************************************/
struct intf_sys_t
{
    vlc_mutex_t         lock;
    vout_thread_t      *p_vout;
    input_thread_t     *p_input;
    int slider_chan;
};

static intf_thread_t *p_intf;

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static int  Open    ( vlc_object_t * );
static void Close   ( vlc_object_t * );

static int msgReceived(void *context, char *topicName, int topicLen, 
    MQTTClient_message *message);

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/

vlc_module_begin ()
    set_shortname( "mqtt" )
    set_description( "mqtt event interface for remote control" )
    set_capability( "interface", 0 )
    set_callbacks( Open, Close )
    set_category( CAT_INTERFACE )
    set_subcategory( SUBCAT_INTERFACE_HOTKEYS )
vlc_module_end ()

static int msgReceived(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    printf("\nMessage arrived new test\n");
    printf("     topic: %s\n", topicName);
    printf("   message: %.*s\n", message->payloadlen, (char *)message->payload);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    printf("%p\n", p_intf);
    playlist_t *p_playlist = pl_Get( p_intf );

    var_TriggerCallback( p_intf->obj.parent, "rate-faster" );
    
    //  DisplayIcon( p_intf->p_sys->p_vout, OSD_MUTE_ICON );
    // var_SetBool( p_playlist, "fullscreen", 1 );
    // playlist_TogglePause( pl_Get( p_intf ) );
    return 1;
}

/*****************************************************************************
 * Open: initialize interface
 *****************************************************************************/
MQTTClient client;
static int Open( vlc_object_t *p_this )
{
    p_intf = (intf_thread_t *)p_this;
    intf_sys_t *p_sys;

    msg_Info(p_this, "going to initializ mqtt...");

    p_sys = malloc( sizeof( intf_sys_t ) );
    if( !p_sys )
        return VLC_ENOMEM;

    p_intf->p_sys = p_sys;
    p_sys->p_vout = NULL;
    p_sys->p_input = NULL;
    
    vlc_mutex_init( &p_sys->lock );
    
    /**
     * Commands that can be executed by mqtt interface to controll the mqtt OSD 
     * overlay filter
    */
    var_Create( p_intf->obj.libvlc, "mqtt-open", VLC_VAR_VOID );
    var_Create( p_intf->obj.libvlc, "mqtt-close", VLC_VAR_VOID );
    var_Create( p_intf->obj.libvlc, "mqtt-left", VLC_VAR_VOID );
    var_Create( p_intf->obj.libvlc, "mqtt-right", VLC_VAR_VOID );
    var_Create( p_intf->obj.libvlc, "mqtt-up", VLC_VAR_VOID );
    var_Create( p_intf->obj.libvlc, "mqtt-down", VLC_VAR_VOID );
    var_Create( p_intf->obj.libvlc, "mqtt-enter", VLC_VAR_VOID );
    var_Create( p_intf->obj.libvlc, "mqtt-volume", VLC_VAR_INTEGER );
    var_Create( p_intf->obj.libvlc, "mqtt-osdclose", VLC_VAR_VOID );

    //Setup mqtt connection here
    
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    
    int rc;

    if ((rc = MQTTClient_create(&client, MQTT_URL, MQTT_CLIENT_ID, 
        MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
    {
        msg_Err(p_this, "failed to create client...\n");
         return VLC_EGENERIC;
    }

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, NULL, msgReceived, NULL);
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        msg_Err(p_this, "Failed to connect, return code %d\n", rc);
        return VLC_EGENERIC;
    }

    MQTTClient_subscribe(client, "vlc", 1);


    return VLC_SUCCESS;
}

/*****************************************************************************
 * Close: destroy interface
 *****************************************************************************/
static void Close( vlc_object_t *p_this )
{
    intf_thread_t *p_intf = (intf_thread_t *)p_this;
    intf_sys_t *p_sys = p_intf->p_sys;

    MQTTClient_disconnect(client, 0);
    MQTTClient_destroy(&client);
    
    var_Destroy( p_intf->obj.libvlc, "mqtt-open");
    var_Destroy( p_intf->obj.libvlc, "mqtt-close");
    var_Destroy( p_intf->obj.libvlc, "mqtt-left");
    var_Destroy( p_intf->obj.libvlc, "mqtt-right");
    var_Destroy( p_intf->obj.libvlc, "mqtt-up");
    var_Destroy( p_intf->obj.libvlc, "mqtt-down");
    var_Destroy( p_intf->obj.libvlc, "mqtt-enter");
    var_Destroy( p_intf->obj.libvlc, "mqtt-volume");
    var_Destroy( p_intf->obj.libvlc, "mqtt-osdclose");

    vlc_mutex_destroy( &p_sys->lock );
    

    free( p_sys );
}

