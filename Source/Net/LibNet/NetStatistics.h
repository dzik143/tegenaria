/******************************************************************************/
/*                                                                            */
/* Copyright (c) 2010, 2014 Sylwester Wysocki <sw143@wp.pl>                   */
/*                                                                            */
/* Permission is hereby granted, free of charge, to any person obtaining a    */
/* copy of this software and associated documentation files (the "Software"), */
/* to deal in the Software without restriction, including without limitation  */
/* the rights to use, copy, modify, merge, publish, distribute, sublicense,   */
/* and/or sell copies of the Software, and to permit persons to whom the      */
/* Software is furnished to do so, subject to the following conditions:       */
/*                                                                            */
/* The above copyright notice and this permission notice shall be included in */
/* all copies or substantial portions of the Software.                        */
/*                                                                            */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    */
/* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    */
/* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        */
/* DEALINGS IN THE SOFTWARE.                                                  */
/*                                                                            */
/******************************************************************************/

#ifndef Tegenaria_Core_NetStatistics_H
#define Tegenaria_Core_NetStatistics_H

#include <Tegenaria/Math.h>
#include <string>

namespace Tegenaria
{
  using std::string;

  //
  // Field set in NetStatistics.fieldsSet_ meaning which struct
  // fields are set.
  //

  #define NET_STAT_FIELD_UPLOAD_SPEED            (1 << 0)
  #define NET_STAT_FIELD_DOWNLOAD_SPEED          (1 << 1)
  #define NET_STAT_FIELD_REQUEST_SPEED           (1 << 2)

  #define NET_STAT_FIELD_BYTES_SENT              (1 << 3)
  #define NET_STAT_FIELD_BYTES_RECV              (1 << 4)

  #define NET_STAT_FIELD_BYTES_UPLOADED          (1 << 5)
  #define NET_STAT_FIELD_BYTES_DOWNLOADED        (1 << 6)

  #define NET_STAT_FIELD_PACKET_SENT_COUNT       (1 << 7)
  #define NET_STAT_FIELD_PACKET_RECV_COUNT       (1 << 8)

  #define NET_STAT_FIELD_REQUEST_COUNT           (1 << 9)
  #define NET_STAT_FIELD_REQUEST_TIME_TOTAL      (1 << 10)
  #define NET_STAT_FIELD_REQUEST_TIME_MAX        (1 << 11)
  #define NET_STAT_FIELD_REQUEST_TIME_AVG        (1 << 12)

  #define NET_STAT_FIELD_PARTIAL_READ_TRIGGERED  (1 << 13)
  #define NET_STAT_FIELD_PARTIAL_WRITE_TRIGGERED (1 << 14)

  #define NET_STAT_FIELD_PING_MAX                (1 << 15)
  #define NET_STAT_FIELD_PING_AVG                (1 << 16)

  //
  // Structure to store network statistics.
  //

  class NetStatistics
  {
    private:

    //
    // Fields.
    //

    MathWeightAvg uploadSpeedAvg_;   // Average upload speed in KB/s
    MathWeightAvg downloadSpeedAvg_; // Average download speed in KB/s
    MathWeightAvg requestTimeAvg_;   // Average request speed in KB/s
    MathWeightAvg requestSpeedAvg_;  // Average request time in ms
    MathWeightAvg pingAvg_;          // Average ping in ms

    double resetTime_;           // Last reset time to compute connection time

    int packetSentCount_;        // Number of all packets sent
    int packetRecvCount_;        // Number of all packets received
    int requestCount_;           // Number of all requests processed.

    double bytesUploaded_;       // Number of bytes uploaded
    double bytesDownloaded_;     // Number of bytes downloaded

    double bytesSent_;           // Total number of bytes sent
    double bytesRecv_;           // Total number of bytes received

    double requestTimeTotal_;    // Total ms spent in processing one request in ms
    double requestTimeMax_;      // Max. time spent in processing one request in ms

    int partialReadTriggered_;   // Read operation was cancelled due to timeout
    int partialWriteTriggered_;  // Write operation was cancelled due to timeout

    double pingMax_;             // Max. noted ping

    unsigned int fieldsSet_;     // Combination of NET_STAT_FIELD_XXX values
                                 // telling which struct fields are set.
    //
    // Functions.
    //

    public:

    NetStatistics();

    string toString();

    void reset();

    double limit(double x);

    //
    // Put data into statistics.
    //

    void insertOutcomingPacket(int size);
    void insertIncomingPacket(int size);

    void insertUploadEvent(int size, double elapsed);
    void insertDownloadEvent(int size, double elapsed);

    void insertRequest(int size, double elapsed);

    void insertPing(double ping);

    void triggerPartialRead();
    void triggerPartialWrite();

    //
    // Getters.
    //

    double getTimeMs();

    double getUploadSpeed();
    double getDownloadSpeed();
    double getRequestTime();
    double getRequestSpeed();
    double getPing();
    double getPingMax();
    double getConnectionTime();

    int getPacketSentCount();
    int getPacketRecvCount();
    int getRequestCount();

    double getBytesUploaded();
    double getBytesDownloaded();
    double getBytesSent();
    double getBytesReceived();
    double getRequestTimeTotal();
    double getRequestTimeMax();

    int isPartialReadTriggered();
    int isPartialWriteTriggered();

    int getNetworkQuality();
  };

} /* namespace Tegenaria */

#endif /* Tegenaria_Core_NetStatistics_H */
