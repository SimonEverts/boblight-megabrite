/*
 * boblight
 * Copyright (C) Bob  2009 
 * 
 * boblight is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * boblight is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "devicembrite.h"
#include "util/log.h"
#include "util/misc.h"
#include "util/timeutils.h"

#include "event_gpio.h"

#include <iostream>
#include <unistd.h>

#define CI  44 // P8_12 -> CI (clock in)
#define SDI 45 // P8_11 -> SDI (data in)
#define LI  26 // P8_14 -> LI (latch in)

CDeviceMBrite::CDeviceMBrite(CClientsHandler& clients) : m_timer(&m_stop), CDevice(clients)
{
}

void CDeviceMBrite::Sync()
{
  if (m_allowsync)
    m_timer.Signal();
}

bool CDeviceMBrite::SetupDevice()
{
  m_timer.SetInterval(m_interval);

  gpio_export(CI);
  gpio_export(SDI);
  gpio_export(LI);

  gpio_set_direction(CI, OUTPUT);
  gpio_set_direction(SDI, OUTPUT);
  gpio_set_direction(LI, OUTPUT);

  if (m_delayafteropen > 0)
    USleep(m_delayafteropen, &m_stop);

  return true;
}

void set_leds (int r, int g, int b)
{
    gpio_set_value(LI, LOW);

    // 2-command bits first, then 10-bit blue, green and red last

    unsigned char command[4];

    command[0] = 0 << 6 | b >> 4; // command + 6 msb bits of blue
    command[1] = b << 4 | r >> 6; // 4 bits of blue + 4 bits of red
    command[2] = r << 2 | g >> 8; // 6 bits of red + 2 bits of green
    command[3] = g; // remaining bits of green

    //cout << (int)command[0] << ";" << (int)command[1] << ";" << (int)command[2] << ";" << (int)command[3] << endl;

    for (int i=0; i<4; i++)
    {
        for (int j=0; j<8; j++)
        {
            gpio_set_value(CI, LOW);
            gpio_set_value(SDI, command[i] & (128 >> j));
            usleep(15);

            gpio_set_value(CI, HIGH);
            usleep(15);
        }

    }

    gpio_set_value(LI, HIGH);
    usleep (15);

    gpio_set_value(LI, LOW);
    usleep(15);

}

bool CDeviceMBrite::WriteOutput()
{
  //get the channel values from the clienshandler
  int64_t now = GetTimeUs();
  m_clients.FillChannels(m_channels, now, this);

  //char colors[3] = {'r', 'g', 'b'};
  //int  color = 0;

  //put channel values in the output buffer
  //for (int i = 0; i < m_channels.size(); i++)
  //{
  //  m_buff[i * 3 + 0] = 0xFF;
  //  m_buff[i * 3 + 1] = colors[color];
  //  m_buff[i * 3 + 2] = value;

  //  color = (color + 1) % 3;
  //}

  int r = Clamp(Round32(m_channels[0].GetValue(now) * 255.0f), 0, 255);
  int g = Clamp(Round32(m_channels[1].GetValue(now) * 255.0f), 0, 255);
  int b = Clamp(Round32(m_channels[2].GetValue(now) * 255.0f), 0, 255);

  set_leds (r, g, b);

  //write output to the serial port
//  if (m_serialport.Write(m_buff, m_channels.size() * 3) == -1)
//  {
//    LogError("%s: %s", m_name.c_str(), m_serialport.GetError().c_str());
//    return false;
//  }
    
  m_timer.Wait(); //wait for the timer to signal us

  return true;
}

void CDeviceMBrite::CloseDevice()
{
    set_leds (0,0,0);

    gpio_unexport(44);
    gpio_unexport(45);
    gpio_unexport(26);
}

