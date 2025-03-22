/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <stdio.h>
 #include <zephyr/kernel.h>
 #include <zephyr/drivers/gpio.h>
 #include <zephyr/sys/printk.h>
 #include <zephyr/bluetooth/bluetooth.h>
 #include <zephyr/bluetooth/hci.h>
 #include <zephyr/bluetooth/conn.h>
 
 #define DEVICE_NAME "nRF52840_Dongle"
 #define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
 #define LED0_NODE DT_ALIAS(led0)
 #define SLEEP_TIME_MS 1000
 
 static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
 static struct k_work_delayable blink_work;
 static bool is_connected = false;
 
 /* Advertising data */
 static const struct bt_data ad[] = {
     BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
     BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
 };
 
 /* Blink LED task */
 static void blink_led(struct k_work *work) {
     if (is_connected) {
         gpio_pin_toggle_dt(&led);
         k_work_reschedule(&blink_work, K_MSEC(SLEEP_TIME_MS));
     }
 }
 
 /* Callback when connected */
 static void connected(struct bt_conn *conn, uint8_t err) {
     if (err) {
         printk("Connection failed (err 0x%02x)\n", err);
     } else {
         printk("Connected\n");
         is_connected = true;
         k_work_reschedule(&blink_work, K_MSEC(SLEEP_TIME_MS));
     }
 }
 
 /* Callback when disconnected */
 static void disconnected(struct bt_conn *conn, uint8_t reason) {
     printk("Disconnected (reason 0x%02x)\n", reason);
     is_connected = false;
     gpio_pin_set_dt(&led, 0);  // Turn off LED when disconnected
 }
 
 /* Bluetooth connection callbacks */
 BT_CONN_CB_DEFINE(conn_callbacks) = {
     .connected = connected,
     .disconnected = disconnected,
 };
 
 int main(void) {
     int err;
 
     printk("Starting BLE advertising on nRF52840 Dongle\n");
 
     if (!gpio_is_ready_dt(&led)) {
         printk("Error: LED device not ready\n");
         return 0;
     }
     gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
 
     k_work_init_delayable(&blink_work, blink_led);
 
     err = bt_enable(NULL);
     if (err) {
         printk("Bluetooth init failed (err %d)\n", err);
         return 0;
     }
 
     printk("Bluetooth initialized\n");
 
     err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
     if (err) {
         printk("Advertising failed to start (err %d)\n", err);
         return 0;
     }
 
     printk("Advertising started successfully\n");
 
     while (1) {
         k_sleep(K_FOREVER);  // Keep the loop alive
     }
 }
 