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

#pragma qcbuild_set_file_title("NetStatistics class")
#pragma qcbuild_set_private(1)

#include <cstdio>
#include <cstring>
#include <sys/time.h>
#include <cmath>
#include "NetStatistics.h"

namespace Tegenaria
{
  #ifdef WIN32
  #define snprintf _snprintf
  #endif

  //
  // Create new clear net statistics with no any fields set.
  //

  NetStatistics::NetStatistics()
  {
    reset();
  }

  //
  // Convert net statistics into human readable string.
  //

  string NetStatistics::toString()
  {
    char buf[128];

    int quality = this -> getNetworkQuality();

    string ret = "Network statistics:\n";

    snprintf(buf, sizeof(buf) - 1, "  Connection time : %lf s.\n", (getTimeMs() - resetTime_) / 1000.0);

    ret += buf;

    if (fieldsSet_ & NET_STAT_FIELD_UPLOAD_SPEED)
    {
      snprintf(buf, sizeof(buf) - 1, "  Upload speed : %lf KB/s.\n", uploadSpeedAvg_.getValue());

      ret += buf;
    }

    if (fieldsSet_ & NET_STAT_FIELD_DOWNLOAD_SPEED)
    {
      snprintf(buf, sizeof(buf) - 1, "  Download speed : %lf KB/s.\n", downloadSpeedAvg_.getValue());

      ret += buf;
    }

    if (fieldsSet_ & NET_STAT_FIELD_BYTES_UPLOADED)
    {
      snprintf(buf, sizeof(buf) - 1, "  Data uploaded : %lf MB.\n", bytesUploaded_ / 1024.0 / 1024.0);

      ret += buf;
    }

    if (fieldsSet_ & NET_STAT_FIELD_BYTES_DOWNLOADED)
    {
      snprintf(buf, sizeof(buf) - 1, "  Data downloaded : %lf MB.\n", bytesDownloaded_ / 1024.0 / 1024.0);

      ret += buf;
    }

    if (fieldsSet_ & NET_STAT_FIELD_BYTES_SENT)
    {
      snprintf(buf, sizeof(buf) - 1, "  Data sent : %lf MB.\n", bytesSent_ / 1024.0 / 1024.0);

      ret += buf;
    }

    if (fieldsSet_ & NET_STAT_FIELD_BYTES_RECV)
    {
      snprintf(buf, sizeof(buf) - 1, "  Data received : %lf MB.\n", bytesRecv_ / 1024.0 / 1024.0);

      ret += buf;
    }

    if (fieldsSet_ & NET_STAT_FIELD_PACKET_SENT_COUNT)
    {
      snprintf(buf, sizeof(buf) - 1, "  Packets sent : %d.\n", packetSentCount_);

      ret += buf;
    }

    if (fieldsSet_ & NET_STAT_FIELD_PACKET_RECV_COUNT)
    {
      snprintf(buf, sizeof(buf) - 1, "  Packets received : %d.\n", packetRecvCount_);

      ret += buf;
    }

    if (fieldsSet_ & NET_STAT_FIELD_REQUEST_COUNT)
    {
      snprintf(buf, sizeof(buf) - 1, "  Requests processed : %d.\n", requestCount_);

      ret += buf;
    }

    if (fieldsSet_ & NET_STAT_FIELD_REQUEST_SPEED)
    {
      snprintf(buf, sizeof(buf) - 1, "  Request speed : %lf KB/s.\n", requestSpeedAvg_.getValue());

      ret += buf;
    }

    if (fieldsSet_ & NET_STAT_FIELD_REQUEST_TIME_TOTAL)
    {
      snprintf(buf, sizeof(buf) - 1, "  Total requests time : %lf s.\n", requestTimeTotal_ / 1000.0);

      ret += buf;
    }

    if (fieldsSet_ & NET_STAT_FIELD_REQUEST_TIME_TOTAL)
    {
      snprintf(buf, sizeof(buf) - 1, "  Max. request time : %lf ms.\n", requestTimeMax_);

      ret += buf;
    }

    if (fieldsSet_ & NET_STAT_FIELD_REQUEST_TIME_AVG)
    {
      snprintf(buf, sizeof(buf) - 1, "  Avg. request time : %lf ms.\n", requestTimeAvg_.getValue());

      ret += buf;
    }

    if (fieldsSet_ & NET_STAT_FIELD_PARTIAL_READ_TRIGGERED)
    {
      snprintf(buf, sizeof(buf) - 1, "  Partial read triggered : %d.\n", partialReadTriggered_);

      ret += buf;
    }

    if (fieldsSet_ & NET_STAT_FIELD_PARTIAL_WRITE_TRIGGERED)
    {
      snprintf(buf, sizeof(buf) - 1, "  Partial write triggered : %d.\n", partialWriteTriggered_);

      ret += buf;
    }

    if (fieldsSet_ & NET_STAT_FIELD_PING_MAX)
    {
      snprintf(buf, sizeof(buf) - 1, "  Max. ping : %lf ms.\n", pingMax_);

      ret += buf;
    }

    if (fieldsSet_ & NET_STAT_FIELD_PING_AVG)
    {
      snprintf(buf, sizeof(buf) - 1, "  Avg. ping : %lf ms.\n", pingAvg_.getValue());

      ret += buf;
    }

    if (quality != -1)
    {
      snprintf(buf, sizeof(buf) - 1, "  Network quality : %d.\n", quality);

      ret += buf;
    }

    return ret;
  }

  //
  // Add one outcoming packet containing <size> bytes to statistics.
  //

  void NetStatistics::insertOutcomingPacket(int size)
  {
    bytesSent_ += size;

    packetSentCount_ ++;

    fieldsSet_ |= NET_STAT_FIELD_BYTES_SENT;
    fieldsSet_ |= NET_STAT_FIELD_PACKET_SENT_COUNT;
  }

  //
  // Add one incoming packet containing <size> bytes to statistics.
  //

  void NetStatistics::insertIncomingPacket(int size)
  {
    bytesRecv_ += size;

    packetRecvCount_ ++;

    fieldsSet_ |= NET_STAT_FIELD_BYTES_RECV;
    fieldsSet_ |= NET_STAT_FIELD_PACKET_RECV_COUNT;
  }

  //
  // Insert one network request to statistics.
  //
  // size    - total request size in bytes i.e. incoming + outcoming (IN).
  // elapsed - time spent to process request in ms (IN).
  //

  void NetStatistics::insertRequest(int size, double elapsed)
  {
    //
    // Update maximum request time.
    //

    if (elapsed > requestTimeMax_)
    {
      requestTimeMax_ = elapsed;
    }

    //
    // Update counter of all processed requests.
    //

    requestCount_ ++;

    requestTimeTotal_ += elapsed;

    //
    // Update average request speed and time.
    //

    requestSpeedAvg_.insert((size / 1024.0) / (elapsed / 1000.0 + 0.1));

    requestTimeAvg_.insert(elapsed);

    //
    // Interpretate small request as ping info.
    //

    if (size < 128)
    {
      insertPing(elapsed);
    }

    //
    // Set that request related fields changed.
    //

    fieldsSet_ |= NET_STAT_FIELD_REQUEST_SPEED;
    fieldsSet_ |= NET_STAT_FIELD_REQUEST_COUNT;
    fieldsSet_ |= NET_STAT_FIELD_REQUEST_TIME_TOTAL;
    fieldsSet_ |= NET_STAT_FIELD_REQUEST_TIME_MAX;
    fieldsSet_ |= NET_STAT_FIELD_REQUEST_TIME_AVG;
  }

  //
  // Mark that partial read occured.
  //

  void NetStatistics::triggerPartialRead()
  {
    partialReadTriggered_ = 1;

    fieldsSet_ |= NET_STAT_FIELD_PARTIAL_READ_TRIGGERED;
  }

  //
  // Mark that partial write occured.
  //

  void NetStatistics::triggerPartialWrite()
  {
    partialWriteTriggered_ = 1;

    fieldsSet_ |= NET_STAT_FIELD_PARTIAL_WRITE_TRIGGERED;
  }

  //
  // Notify that we uploaded <size> bytes in <elapsed> time.
  //
  // size    - number of bytes uploaded (IN).
  // elapsed - upload time in ms (IN).
  //

  void NetStatistics::insertUploadEvent(int size, double elapsed)
  {
    bytesUploaded_ += size;

    uploadSpeedAvg_.insert((size / 1024.0) / (elapsed / 1000.0 + 0.1));

    fieldsSet_ |= NET_STAT_FIELD_BYTES_UPLOADED;
    fieldsSet_ |= NET_STAT_FIELD_UPLOAD_SPEED;
  }

  //
  // Notify that we downloaded <size> bytes in <elapsed> time.
  //
  // size    - number of bytes downloaded (IN).
  // elapsed - upload time in ms (IN).
  //

  void NetStatistics::insertDownloadEvent(int size, double elapsed)
  {
    bytesDownloaded_ += size;

    downloadSpeedAvg_.insert((size / 1024.0) / (elapsed / 1000.0 + 0.1));

    fieldsSet_ |= NET_STAT_FIELD_BYTES_DOWNLOADED;
    fieldsSet_ |= NET_STAT_FIELD_DOWNLOAD_SPEED;
  }

  //
  // Insert current ping value.
  //
  // ping - noted ping value in ms (IN).
  //

  void NetStatistics::insertPing(double ping)
  {
    if (ping > pingMax_)
    {
      pingMax_ = ping;
    }

    pingAvg_.insert(ping);

    fieldsSet_ |= NET_STAT_FIELD_PING_MAX;
    fieldsSet_ |= NET_STAT_FIELD_PING_AVG;
  }


  //
  // Clear all fields.
  //

  void NetStatistics::reset()
  {
    resetTime_ = getTimeMs();

    uploadSpeedAvg_.clear();
    downloadSpeedAvg_.clear();
    requestTimeAvg_.clear();
    requestSpeedAvg_.clear();
    pingAvg_.clear();

    packetSentCount_  = 0;
    packetRecvCount_  = 0;
    requestCount_     = 0;

    bytesSent_ = 0.0;
    bytesRecv_ = 0.0;

    bytesDownloaded_ = 0.0;
    bytesUploaded_   = 0.0;

    requestTimeTotal_ = 0.0;
    requestTimeMax_   = 0.0;

    partialReadTriggered_  = 0;
    partialWriteTriggered_ = 0;

    pingMax_ = 0.0;

    fieldsSet_ = 0;
  }

  //
  // ----------------------------------------------------------------------------
  //
  //                                 Getters
  //
  // ----------------------------------------------------------------------------
  //

  double NetStatistics::getTimeMs()
  {
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
  }

  double NetStatistics::getUploadSpeed()
  {
    return uploadSpeedAvg_.getValue();
  }

  double NetStatistics::getDownloadSpeed()
  {
    return downloadSpeedAvg_.getValue();
  }

  double NetStatistics::getRequestTime()
  {
    return requestTimeAvg_.getValue();
  }

  double NetStatistics::getRequestSpeed()
  {
    return requestSpeedAvg_.getValue();
  }

  double NetStatistics::getPing()
  {
    return pingAvg_.getValue();
  }

  double NetStatistics::getPingMax()
  {
    return pingMax_;
  }

  //
  // Get connection time in seconds.
  //

  double NetStatistics::getConnectionTime()
  {
    return (resetTime_ - getTimeMs()) / 1000.0;
  }

  int NetStatistics::getPacketSentCount()
  {
    return packetSentCount_;
  }

  int NetStatistics::getPacketRecvCount()
  {
    return packetSentCount_;
  }

  int NetStatistics::getRequestCount()
  {
    return packetSentCount_;
  }

  double NetStatistics::getBytesUploaded()
  {
    return bytesUploaded_;
  }

  double NetStatistics::getBytesDownloaded()
  {
    return bytesUploaded_;
  }

  double NetStatistics::getBytesSent()
  {
    return bytesSent_;
  }

  double NetStatistics::getBytesReceived()
  {
    return bytesRecv_;
  }

  //
  // Get total time spent in processing requests in seconds.
  //

  double NetStatistics::getRequestTimeTotal()
  {
    return requestTimeTotal_ / 1000.0;
  }

  //
  // Get maximum time spent to process one request.
  //

  double NetStatistics::getRequestTimeMax()
  {
    return requestTimeMax_;
  }

  int NetStatistics::isPartialReadTriggered()
  {
    return partialReadTriggered_;
  }

  int NetStatistics::isPartialWriteTriggered()
  {
    return partialWriteTriggered_;
  }

  //
  // Limit input value to fit <0;1> range.
  // Used internally only.
  //

  double NetStatistics::limit(double x)
  {
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;

    return x;
  }

  //
  // Compute network quality in 1-5 scale.
  //
  // RETURNS: Network quality in <1;5> range,
  //          or -1 if too less data to estimate network quality.
  //

  int NetStatistics::getNetworkQuality()
  {
    double wsum = 0.0;

    double sum = 0.0;

    //
    // Estimate average request time quality.
    //

    if (fieldsSet_ & NET_STAT_FIELD_REQUEST_TIME_AVG)
    {
      sum += (1.0 - limit(getRequestTime() / 500.0)) * 1.0;

      wsum += 1.0;
    }

    //
    // Estimate download quality.
    //

    if (fieldsSet_ & NET_STAT_FIELD_DOWNLOAD_SPEED)
    {
      sum += (1.0 - limit(getDownloadSpeed() / 10000.0)) * 0.5;

      wsum += 0.5;
    }

    //
    // Estimate upload quality.
    //

    if (fieldsSet_ & NET_STAT_FIELD_UPLOAD_SPEED)
    {
      sum += (1.0 - limit(getUploadSpeed() / 10000.0)) * 0.5;

      wsum += 0.5;
    }

    //          requestTimeMax
    // Estimate -------------- ratio quality.
    //          requestTimeAvg
    //

    if (fieldsSet_ & NET_STAT_FIELD_REQUEST_TIME_MAX
            && fieldsSet_ & NET_STAT_FIELD_REQUEST_TIME_AVG
              && getRequestTime() > 100.0)
    {
      sum += (1.0 - limit((getRequestTimeMax() / getRequestTime()) / 4.0)) * 0.5;

      wsum += 0.5;
    }

    //
    // Estimate ping quality.
    //

    if (fieldsSet_ & NET_STAT_FIELD_PING_AVG)
    {
      sum += (1.0 - limit(getPing() / 200)) * 1.0;

      wsum += 1.0;
    }

    //
    // Decrease quality if partial read/write occures.
    //

    if (isPartialReadTriggered() || isPartialWriteTriggered())
    {
      wsum += 2.0;
    }

    //
    // Compute total quality from partials.
    //

    if (wsum > 0.0)
    {
      return round((sum / wsum) * 5.0);
    }
    else
    {
      return -1;
    }
  }
} /* namespace Tegenaria */
