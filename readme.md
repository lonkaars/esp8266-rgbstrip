this is a work-in-progress

- am using [espressif's ESP8266_RTOS_SDK](https://github.com/espressif/ESP8266_RTOS_SDK)
- run `make compile_commands` if you're using the clangd language server
- need to add/modify the following in your sdkconfig:
	```bash
	# compilation on arch
	CONFIG_SDK_PYTHON="python2"

	# wifi settings
	CONFIG_EXAMPLE_WIFI_SSID="your network name"
	CONFIG_EXAMPLE_WIFI_PASSWORD="your network password"

	# hostname (optional)
	CONFIG_LWIP_LOCAL_HOSTNAME="your custom hostname instead of espressif"
	```
