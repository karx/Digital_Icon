#include <ota.h>
#include <Update.h>

int _contentLength = 0;
bool _isValidContentType = false;

String _getHeaderValue(String header, String headerName)
{
  return header.substring(strlen(headerName.c_str()));
}

void OTA_ESP32::execOTA(String host, int port, String bin, WiFiClient *wifiClient)
    {
        Serial.println("Connecting to: " + String(host));
        // Connect to S3
        if (wifiClient->connect(host.c_str(), port))
        {
            // Connection Succeed.
            // Fecthing the bin
            Serial.println("Fetching Bin: " + String(bin));

            // Get the contents of the bin file
            wifiClient->print(String("GET ") + bin + " HTTP/1.1\r\n" +
                             "Host: " + host + "\r\n" +
                             "Cache-Control: no-cache\r\n" +
                             "Connection: close\r\n\r\n");

            // Check what is being sent
            Serial.print(String("GET ") + bin + " HTTP/1.1\r\n" +
                         "Host: " + host + "\r\n" +
                         "Cache-Control: no-cache\r\n" +
                         "Connection: close\r\n\r\n");

            unsigned long timeout = millis();
            while (wifiClient->available() == 0)
            {
                if (millis() - timeout > 5000)
                {
                    Serial.println("Client Timeout !");
                    wifiClient->stop();
                    return;
                }
            }
            // Once the response is available,
            // check stuff

            /*
       Response Structure
        HTTP/1.1 200 OK
        x-amz-id-2: NVKxnU1aIQMmpGKhSwpCBh8y2JPbak18QLIfE+OiUDOos+7UftZKjtCFqrwsGOZRN5Zee0jpTd0=
        x-amz-request-id: 2D56B47560B764EC
        Date: Wed, 14 Jun 2017 03:33:59 GMT
        Last-Modified: Fri, 02 Jun 2017 14:50:11 GMT
        ETag: "d2afebbaaebc38cd669ce36727152af9"
        Accept-Ranges: bytes
        Content-Type: application/octet-stream
        Content-Length: 357280
        Server: AmazonS3
                                   
        {{BIN FILE CONTENTS}}
    */
            while (wifiClient->available())
            {
                // read line till /n
                String line = wifiClient->readStringUntil('\n');
                // remove space, to check if the line is end of headers
                line.trim();

                // if the the line is empty,
                // this is end of headers
                // break the while and feed the
                // remaining `client` to the
                // Update.writeStream();
                if (!line.length())
                {
                    //headers ended
                    break; // and get the OTA started
                }

                // Check if the HTTP Response is 200
                // else break and Exit Update
                if (line.startsWith("HTTP/1.1"))
                {
                    if (line.indexOf("200") < 0)
                    {
                        Serial.println("Got a non 200 status code from server. Exiting OTA Update.");
                        break;
                    }
                }

                // extract headers here
                // Start with content length
                if (line.startsWith("Content-Length: "))
                {
                    _contentLength = atoi((_getHeaderValue(line, "Content-Length: ")).c_str());
                    Serial.println("Got " + String(_contentLength) + " bytes from server");
                }

                // Next, the content type
                if (line.startsWith("Content-Type: "))
                {
                    String contentType = _getHeaderValue(line, "Content-Type: ");
                    Serial.println("Got " + contentType + " payload.");
                    if (contentType == "application/octet-stream")
                    {
                        _isValidContentType = true;
                    }
                }
            }
        }
        else
        {
            // Connect to S3 failed
            // May be try?
            // Probably a choppy network?
            Serial.println("Connection to " + String(host) + " failed. Please check your setup");
            // retry??
            // execOTA();
        }

        // Check what is the _contentLength and if content type is `application/octet-stream`
        Serial.println("_contentLength : " + String(_contentLength) + ", _isValidContentType : " + String(_isValidContentType));

        // check _contentLength and content type
        if (_contentLength && _isValidContentType)
        {
            // Check if there is enough to OTA Update
            bool canBegin = Update.begin(_contentLength);

            // If yes, begin
            if (canBegin)
            {
                Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
                // No activity would appear on the Serial monitor
                // So be patient. This may take 2 - 5mins to complete
                size_t written = Update.writeStream(*wifiClient);

                if (written == _contentLength)
                {
                    Serial.println("Written : " + String(written) + " successfully");
                }
                else
                {
                    Serial.println("Written only : " + String(written) + "/" + String(_contentLength) + ". Retry?");
                    // retry??
                    // execOTA();
                }

                if (Update.end())
                {
                    Serial.println("OTA done!");
                    if (Update.isFinished())
                    {
                        Serial.println("Update successfully completed. Rebooting.");
                        ESP.restart();
                    }
                    else
                    {
                        Serial.println("Update not finished? Something went wrong!");
                    }
                }
                else
                {
                    Serial.println("Error Occurred. Error #: " + String(Update.getError()));
                }
            }
            else
            {
                // not enough space to begin OTA
                // Understand the partitions and
                // space availability
                Serial.println("Not enough space to begin OTA");
                wifiClient->flush();
            }
        }
        else
        {
            Serial.println("There was no content in the response");
            wifiClient->flush();
        }
    }
 