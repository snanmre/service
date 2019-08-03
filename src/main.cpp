#include <iostream>
#include <sstream>
#include <cerrno>
#include <cstring>

#include <unistd.h>

#include <Debug.h>
#include "service_server.h"
#include "service_client.h"

#include "ServiceMessages.h"

using namespace std;

//int test_exec();

/**
 * @author Sinan Emre Kutlu
 *
 * @date 2017-08-23
 *
 * @mainpage service
 *
 * @section Açıklama Açıklama
 *
 * Bu uygulama, tanımlanan config dosyalarına göre diğer uygulamaları daemon olarak başlatabilir,
 * log kayıtlarını tutabilir ve öldüklerinde yeniden başlatabilir. Genel haliyle upstart yapısının
 * basitleştirilmiş halidir.
 *
 * @section Detay Detay
 *
 * Bir servisin başlatılması istenildiğinde, aynı isimdeki config dosyası okunur ve servise ait
 * komut(veya script), tanımlanmışsa log dosyası, öldüğü durumda ne yapılacağı ile ilgili tercihler
 * belirlenir. Daha sonra tanımlanan formata uygun ise servis başlatılır. Servis öldüğü takdirde eğer
 * istenmişse yeniden başlatılır.
 *
 * @section related_files İlgili Dosyalar
 *
 * - DIRPATH_SERVICES      		: servislere ait config dosyalarının bulunduğu dizin
 * - DIRPATH_SERVICE_PIDS       : servislere ait pid dosyalarının bulunduğu dizin
 * - FILEPATH_SERVICES_LIST   	: çalışan servislerin listesini içeren dosya
 *
 * @section config_file_format Config Dosyası Formatı
 *
 *
 * @code
 * # execute command
 *
 * exec <command>
 *
 * # execute multi-line script
 *
 * script
 *     ...line 1...
 *     ...line 2...
 *     ...line 3...
 * end script
 *
 *
 * # log file path (default: /dev/null)
 *
 * log <log-file-path>
 *
 *
 * # wipe log on start/restart
 *
 * wipe log
 *
 *
 * # pid file path (default: /run/service/<service_name>.pid)
 *
 * pidfile <pid-file-path>
 *
 *
 * # respawn service if it dies
 *
 * respawn
 *
 *
 * # set respawn options
 * # limit: respawn limit count, default: int max
 * # interval: delay before start in seconds, default: 0
 *
 * respawn limit <limit>  <interval>
 *
 * @endcode
 */

int main(int argc, char * argv[])
{
	service_client sc;
	return sc.process(argc, argv);
}

