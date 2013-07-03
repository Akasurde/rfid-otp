/*
    rfid-otp - An HMAC-OTP RFID token
    Copyright (C) 2013 Jon Kivinen 

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

t_max = 3;
t_min = 1;

dims = [85.60, 53.98, t_max];

linear_extrude(file = "rfid.dxf",
               layer = "logo",
               height = (t_max+t_min)/2);

linear_extrude(file = "rfid.dxf",
               layer = "border",
               height = t_max);

linear_extrude(file = "rfid.dxf",
               layer = "winding_support",
               height = t_max);

linear_extrude(file = "rfid.dxf",
               layer = "pcb_flanges",
               height = t_min);

linear_extrude(file = "rfid.dxf",
               layer = "base",
               height = t_min);
