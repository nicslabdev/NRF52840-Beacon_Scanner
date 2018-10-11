/***************************************************************************************/
/*
 * motam_scanner
 * Created by Manuel Montenegro, Oct 8, 2018.
 * Developed for MOTAM project.
 *
 *  This is a Bluetooth 5 scanner. This code reads every advertisement from MOTAM beacons
 *  and sends its data through serial port.
 *
 *  This code has been developed for Nordic Semiconductor nRF52840 PDK & nRF52840 dongle.
*/
/***************************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_drv_usbd.h"
#include "nrf_drv_clock.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_drv_power.h"

#include "app_error.h"
#include "app_util.h"
#include "app_usbd_core.h"
#include "app_usbd.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_serial_num.h"

#include "boards.h"
#include "bsp.h"
#include "bsp_cli.h"
#include "nrf_cli.h"
#include "nrf_cli_uart.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "ble.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "nrf_ble_scan.h"

#include "nrf_fstorage.h"


// ======== USB Device CDC ACM configuration ========

// Enable power USB detection
#ifndef USBD_POWER_DETECTION
#define USBD_POWER_DETECTION true
#endif

// USB Device CDC ACM board configuration parameters
#define CDC_ACM_COMM_INTERFACE  0
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN2
#define CDC_ACM_DATA_INTERFACE  1
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT1

// USB Device CDC ACM user event handler
static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_cdc_acm_user_event_t event);

// USB Device CDC ACM instance
APP_USBD_CDC_ACM_GLOBAL_DEF(m_app_cdc_acm,
                            cdc_acm_user_ev_handler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250
);

// USB Device CDC ACM transmission buffers
static char m_tx_buffer[255*2];

// USB Device CDC ACM user event handler
static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_cdc_acm_user_event_t event)
{
    switch (event)
    {
        case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
            break;
        case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
            break;
        case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
            break;
        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
            break;
        default:
            break;
    }
}

// USB Device event handler
static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
    switch (event)
    {
        case APP_USBD_EVT_DRV_SUSPEND:
            break;
        case APP_USBD_EVT_DRV_RESUME:
            break;
        case APP_USBD_EVT_STARTED:
            break;
        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
            break;
        case APP_USBD_EVT_POWER_DETECTED:
            if (!nrf_drv_usbd_is_enabled())
            {
                app_usbd_enable();
            }
            break;
        case APP_USBD_EVT_POWER_REMOVED:
            app_usbd_stop();
            break;
        case APP_USBD_EVT_POWER_READY:
            app_usbd_start();
            break;
        default:
            break;
    }
}

// USB Device initialization
static void usbd_init (void)
{
	ret_code_t err_code;

	app_usbd_serial_num_generate();

	static const app_usbd_config_t usbd_config = {
		.ev_state_proc = usbd_user_ev_handler
	};

	err_code = app_usbd_init(&usbd_config);
	APP_ERROR_CHECK(err_code);

	app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
	err_code = app_usbd_class_append(class_cdc_acm);
	APP_ERROR_CHECK(err_code);
}

// Function that enables USB Device events. This function must be called after BLE functions starts
static void usbd_start (void)
{
	ret_code_t err_code;
	err_code = app_usbd_power_events_enable();
	APP_ERROR_CHECK(err_code);
}


// ======== BLE configuration ========

// BLE configuration parameters
#define APP_BLE_CONN_CFG_TAG		1						// Tag that identifies the BLE configuration of the SoftDevice.
#define APP_BLE_OBSERVER_PRIO		3						// BLE observer priority of the application. There is no need to modify this value.
#define APP_SOC_OBSERVER_PRIO		1						// SoC observer priority of the application. There is no need to modify this value.

#define SCAN_INTERVAL               0x0320					// Determines scan interval in units of 0.625 millisecond.
#define SCAN_WINDOW                 0x0320					// Determines scan window in units of 0.625 millisecond.
#define SCAN_DURATION				0x0000					// Duration of the scanning in units of 10 milliseconds. If set to 0x0000, scanning continues until it is explicitly disabled.

static bool							m_mem_acc_in_progress;	// Flag to keep track of ongoing operations on persistent memory.

static ble_gap_scan_params_t m_scan_param =					// Scan parameters requested for scanning and connection.
{
    .active        = 0x00,
    .interval      = SCAN_INTERVAL,
    .window        = SCAN_WINDOW,
    .filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL,
    .timeout       = SCAN_DURATION,
    .scan_phys     = BLE_GAP_PHY_CODED,						// Here you can select the PHY (BLE_GAP_PHY_CODED or BLE_GAP_PHY_1MBPS)
    .extended      = 1,
};

// Scanning Module instance.
NRF_BLE_SCAN_DEF (m_scan);

// Function for starting scanner
static void scan_start(void);

// Function for handling Scanning Module events
static void scan_evt_handler(scan_evt_t const * p_scan_evt)
{

    switch(p_scan_evt->scan_evt_id)
    {
        case NRF_BLE_SCAN_EVT_SCAN_TIMEOUT:
        {
            scan_start();
        }
        break;

        // No whitelist set up, so the filter is not matched for the scan data.
        case NRF_BLE_SCAN_EVT_NOT_FOUND:
        {
        	int i;											// Index for loop
        	uint8_t * p_data = p_scan_evt->params.p_not_found->data.p_data;	// Pointer to packet data
			int data_len = p_scan_evt->params.p_not_found->data.len;	// Packet data length

			// Filtering MOTAM advertisements
        	if (p_data[1] == 0xFF && p_data[2] == 0xBE && p_data [3] == 0xDE)
        	{
				// Conversion from byte string to hexadecimal char string
				for (i = 0; i < data_len; i++ )
				{
					sprintf(&m_tx_buffer[i*2], "%02x", p_data[i]);
				}
				sprintf(&m_tx_buffer[data_len*2], "\r\n");

				// Send the hexadecimal char string by serial port
				app_usbd_cdc_acm_write(&m_app_cdc_acm, m_tx_buffer, (data_len*2)+2);
        	}
        }
        break;

        default:
        	break;
    }
}

// BLE events handler
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_ADV_REPORT:
        	break;

        default:
            break;
    }
}

// SoftDevice SoC event handler.
static void soc_evt_handler(uint32_t evt_id, void * p_context)
{
	switch (evt_id)
    {
        case NRF_EVT_FLASH_OPERATION_SUCCESS:
        /* fall through */
        case NRF_EVT_FLASH_OPERATION_ERROR:

            if (m_mem_acc_in_progress)
            {
                m_mem_acc_in_progress = false;
                scan_start();
            }
            break;

        default:
            // No implementation needed.
            break;
    }
}

// Function for initializing the BLE stack
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register handlers for BLE and SoC events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
    NRF_SDH_SOC_OBSERVER(m_soc_observer, APP_SOC_OBSERVER_PRIO, soc_evt_handler, NULL);
}

// Function for initializing the scanning and setting the filters
static void scan_init(void)
{
    ret_code_t          err_code;
    nrf_ble_scan_init_t init_scan;

    memset(&init_scan, 0, sizeof(init_scan));

    init_scan.connect_if_match = false;
    init_scan.conn_cfg_tag     = APP_BLE_CONN_CFG_TAG;
    init_scan.p_scan_param     = &m_scan_param;

    err_code = nrf_ble_scan_init(&m_scan, &init_scan, scan_evt_handler);
    APP_ERROR_CHECK(err_code);
}

// Function for starting scanning
static void scan_start(void)
{
    ret_code_t err_code;

    // If there is any pending write to flash, defer scanning until it completes.
    if (nrf_fstorage_is_busy(NULL))
    {
        m_mem_acc_in_progress = true;
        return;
    }

    err_code = nrf_ble_scan_start(&m_scan);
    APP_ERROR_CHECK(err_code);
}


// ======== Board Initialization ========

// Logging initialization
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

// Clock initialization
static void clock_init (void)
{
	ret_code_t err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);

    nrf_drv_clock_lfclk_request(NULL);

    while(!nrf_drv_clock_lfclk_is_running())
    {
        /* Just waiting */
    }
}

// Timer initialization
static void timer_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

// Power managemente initialization
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

// Function for handling the idle state (main loop)
static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}


int main(void)
{

	// Initialize.
	log_init();
	timer_init();
	clock_init();
	usbd_init();
	power_management_init();
	ble_stack_init();
	scan_init();

	scan_start();
	usbd_start();

    // Start execution.
    NRF_LOG_RAW_INFO(    "  -------------\r\n");
    NRF_LOG_RAW_INFO(	 "| MOTAM scanner |");
    NRF_LOG_RAW_INFO("\r\n  -------------\r\n");

	for (;;)
	{
		while (app_usbd_event_queue_process())
		{
			// While there are USBD events in queue...
		}
		idle_state_handle();
	}
}
