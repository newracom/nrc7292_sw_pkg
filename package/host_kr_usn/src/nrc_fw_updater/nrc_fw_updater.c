#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#include <curl/curl.h>
#include <cjson/cJSON.h>

#define VERSION_STRING_SIZE 32

/* firmware server url */
char *firmware_server = NULL;

/* firmware version information file in server */
char *firmware_info = NULL;

/* location where the SDK is installed */
char *sdk_root = NULL;

/* location where SDK version file is in */
char *installed_sdk_version = NULL;

/* scheduled time in hour. i.e 3.5 will be 3:30 */
/* if negative number is given, it will run once and exit */
float scheduled = 0.0;

/* 0:STA   |  1:AP  */
unsigned int station_type = 0;

/* security mode 0:Open  |  1:WPA2-PSK  |  2:WPA3-OWE  |  3:WPA3-SAE */
unsigned int security_mode = 0;

/* US:USA  |  JP:Japan  |  TW:Taiwan  | KR:Korea | EU:EURO | CN:China */
char *country = NULL;

struct curl_data {
	char *payload;
	size_t size;
};

static int current_time_in_second()
{
	time_t raw_time;
	struct tm *time_info;
	time(&raw_time);
	time_info = localtime(&raw_time);

	return time_info->tm_hour * 60 * 60 + time_info->tm_min * 60 + time_info->tm_sec;
}

/* hour in 0 - 23 */
static int sleep_until(float hour)
{
	int current_time = current_time_in_second();
	int wakeup_time = (int) (hour * 60 * 60);
	int sleep_time = wakeup_time - current_time;
	int rand = random() % (5 * 60);

	if (sleep_time < 0) {
		sleep_time = (wakeup_time + 60 * 60 * 24) - current_time;
	}

	printf("[%s] sleeping for %d\n", __func__, sleep_time + rand);
	sleep(sleep_time + rand);
}

static size_t curl_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct curl_data *data = (struct curl_data *) userp;

    data->payload = (char *) realloc(data->payload, realsize + 1);

    if (data->payload == NULL) {
        printf("[%s] payload null", __func__);
        free(data->payload);
        return 0;
    }

    memcpy(&(data->payload[data->size]), contents, realsize);

    data->size += realsize;
    data->payload[data->size] = 0;
    return realsize;
}

int get_json_str_value(cJSON *cjson, char *key, char **value)
{
    cJSON *cjson_obj = NULL;

    cjson_obj = cJSON_GetObjectItem(cjson, key);
    if (cjson_obj && cjson_obj->valuestring) {
        *value = strdup(cjson_obj->valuestring);
        return 1;
    } else {
        printf("[%s] %s not found in json\n", __func__, key);
        return 0;
    }
}

int curl_download_fw_info(char **version, char **md5sum, char **location)
{
	int result = 0;

	struct curl_data fetch = {0};

	CURL *curl = NULL;
	struct curl_slist *slist = NULL;
	CURLcode response = CURLE_OK;

	char url[256] = {0};
	char buffer[1024] = {0};

	cJSON *cjson = NULL;

	curl = curl_easy_init();
	if (curl == NULL) {
		result = -1;
		goto exit_below;
	}

	snprintf(buffer, sizeof(buffer), "%s: %s", "Content-Type", "application/json;charset=UTF-8");
	slist = curl_slist_append(slist, buffer);
	snprintf(buffer, sizeof(buffer), "%s: %s", "Accept", "application/json");
	slist = curl_slist_append(slist, buffer);

	sprintf(url, "%s/%s", firmware_server, firmware_info);
	curl_easy_setopt(curl, CURLOPT_URL, url);

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &fetch);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
//	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 60L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

	response = curl_easy_perform(curl);

	curl_slist_free_all(slist);

	if (response != CURLE_OK || fetch.payload == NULL) {
        printf("[%s] error: failed to fetch url (%s) - %s\n", __func__, url, curl_easy_strerror(response));
		result = -1;
        goto exit_below;
    }

    cjson = cJSON_Parse(fetch.payload);

    if (!cjson) {
        printf("[%s] response parse error\n", __func__);
		result = -1;
        goto exit_below;
    }

	if (get_json_str_value(cjson, "version", version)) {
		printf("[%s] Version : %s\n", __func__, *version);
	} else {
		result = -1;
		goto exit_below;
	}

	if (get_json_str_value(cjson, "md5sum", md5sum)) {
		printf("[%s] md5sum : %s\n", __func__, *md5sum);
	} else {
		result = -1;
		goto exit_below;
	}

	if (get_json_str_value(cjson, "url", location)) {
		printf("[%s] URL : %s\n", __func__, *location);
	} else {
		result = -1;
		goto exit_below;
	}

exit_below:
	if (cjson) {
		cJSON_Delete(cjson);
	}

	if (fetch.payload) {
		free(fetch.payload);
	}

	if (curl) {
		curl_easy_cleanup(curl);
	}

	return result;
}

char *nrc_sdk_version(char *dir, char *file)
{
	char *version_string = NULL;
	FILE *version_file = NULL;

	int major;
	int minor;
	int revision;

	char *path = NULL;

	/* 2 : "/" and blank */
	path = (char *) malloc(strlen(dir) + strlen(file) + 2);
	if (path == NULL) {
		printf("[%s] path %s/%s malloc failed.\n", __func__, dir, file);
		return NULL;
	}

	sprintf(path, "%s/%s", dir, file);
	if ((version_file = fopen(path, "r")) == 0) {
		printf("[%s] opening %s failed\n", __func__, file);
		return NULL;
	}

	if (fscanf(version_file, "VERSION_MAJOR %d\n", &major) == 0) {
		printf("[%s] VERSION_MAJOR not found\n", __func__);
		goto error_return;
	}

	if (fscanf(version_file, "VERSION_MINOR %d\n", &minor) == 0) {
		printf("[%s] VERSION_MINOR not found\n", __func__);
		goto error_return;
	}

	if (fscanf(version_file, "VERSION_REVISION %d\n", &revision) == 0) {
		printf("[%s] VERSION_REVISION not found\n", __func__);
		goto error_return;
	}

	printf("version : %d.%d.%d\n", major, minor, revision);

	version_string = (char *) malloc(VERSION_STRING_SIZE);
	if (!version_string) {
		goto error_return;
	}

	snprintf(version_string, VERSION_STRING_SIZE, "%d.%d.%d", major, minor, revision);

error_return:
	if (version_file) {
		fclose(version_file);
	}
	if (path) {
		free(path);
	}
	return version_string;
}

int process_config(char *config)
{
	int result = 0;
	FILE *f = NULL;
    long length = 0;
    char *data = NULL;

	cJSON *cjson = NULL;
	char *scheduled_at = NULL;
	char *station = NULL;
	char *security = NULL;

    /* open in read binary mode */
    f = fopen(config, "rb");
	if (f == NULL) {
		printf("[%s] error opening %s\n", __func__, config);
		return -1;
	}

    /* get the length */
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);

    data = (char*)malloc(length + 1);

	if (data == NULL) {
		printf("[%s] error allocating memory...\n", __func__);
		fclose(f);
		return -1;
	}

    fread(data, 1, length, f);
    data[length] = 0;

    fclose(f);

	cjson = cJSON_Parse(data);
	if (cjson == NULL) {
		printf("[%s] error parsing %s\n", __func__, config);
		return -1;
	} else {
		if (get_json_str_value(cjson, "firmware_server", &firmware_server)) {
			printf("[%s] firmware server : %s\n", __func__, firmware_server);
		} else {
			result = -1;
			goto exit_below;
		}

		if (get_json_str_value(cjson, "firmware_info", &firmware_info)) {
			printf("[%s] firmware info file : %s\n", __func__, firmware_info);
		} else {
			result = -1;
			goto exit_below;
		}

		if (get_json_str_value(cjson, "sdk_root", &sdk_root)) {
			printf("[%s] installed sdk root : %s\n", __func__, sdk_root);
		} else {
			result = -1;
			goto exit_below;
		}

		if (get_json_str_value(cjson, "installed_sdk_version", &installed_sdk_version)) {
			printf("[%s] installed sdk version file : %s\n", __func__, installed_sdk_version);
		} else {
			result = -1;
			goto exit_below;
		}

		if (get_json_str_value(cjson, "schedule", &scheduled_at)) {
			printf("[%s] scheduled at : %s\n", __func__, scheduled_at);
			scheduled = atof(scheduled_at);
		} else {
			result = -1;
			goto exit_below;
		}

		if (get_json_str_value(cjson, "station_type", &station)) {
			printf("[%s] station type : %s\n", __func__, station);
		} else {
			result = -1;
			goto exit_below;
		}

		if (get_json_str_value(cjson, "security_mode", &security)) {
			printf("[%s] security mode : %s\n", __func__, security);
			if (strcasecmp(security, "open") == 0) {
				security_mode = 0;
			} else if (strcasecmp(security, "WPA2-PSK") == 0) {
				security_mode = 1;
			} else if (strcasecmp(security, "WPA3-OWE") == 0) {
				security_mode = 2;
			} else if (strcasecmp(security, "WPA3-SAE") == 0) {
				security_mode = 3;
			} else {
				result = -1;
				goto exit_below;
			}
		} else {
			result = -1;
			goto exit_below;
		}

		if (get_json_str_value(cjson, "country", &country)) {
			printf("[%s] country : %s\n", __func__, country);
		} else {
			result = -1;
			goto exit_below;
		}
	}

exit_below:
	if (data) {
		free(data);
	}
	if (scheduled_at) {
		free(scheduled_at);
	}
	if (station) {
		free(station);
	}
	if (security) {
		free(security);
	}
	if (cjson) {
		cJSON_Delete(cjson);
	}
	return result;
}

int main(int argc, char *argv[])
{
	char *version = NULL;
	char *md5sum = NULL;
	char *location = NULL;

	char *sdk_version = NULL;

	int compare = 0;

	if (argc != 2) {
		printf("Usage: %s config.json\n", argv[0]);
		return 1;
	}

	if (process_config(argv[1]) < 0) {
		printf("Error processing config file %s...\n", argv[1]);
		return 1;
	}

	do {
		if (scheduled > 0.0) {
			sleep_until(scheduled);
		}

		if ((sdk_version = nrc_sdk_version(sdk_root, installed_sdk_version)) == NULL) {
			printf("%s not found\n", installed_sdk_version);
			return -1;
		}

		if (curl_download_fw_info(&version, &md5sum, &location) == 0) {
			printf("received version : %s, md5sum : %s, location : %s\n", version, md5sum, location);

			compare = strverscmp(sdk_version, version);
			if (compare > 0) {
				printf("installed version higher than fw available\n");
			} else if (compare == 0) {
				printf("same version installed\n");
			} else {
				printf("Upgrade needed\n");
				char cmd[256] = {0};
				sprintf(cmd, "bash %s/script/nrc_fw_update.sh %s %s %s %s %d %d %s",
						sdk_root, version, md5sum, location, sdk_root,
						station_type, security_mode, country);
				system(cmd);
			}
		}

		if (sdk_version) {
			free(sdk_version);
		}

		if (version) {
			free(version);
		}

		if (md5sum) {
			free(md5sum);
		}

		if (location) {
			free(location);
		}
	} while (scheduled > 0.0);

	if (firmware_server) {
		free(firmware_server);
	}

	if (firmware_info) {
		free(firmware_info);
	}

	if (sdk_root) {
		free(sdk_root);
	}

	if (installed_sdk_version) {
		free(installed_sdk_version);
	}

	if (country) {
		free(country);
	}

	return 0;
}
