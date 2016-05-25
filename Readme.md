# Direct Fourier Transformation of Atom probe data

This program is written to perform a 3D Direct Fourier Transformation (**no** Fast Fourier) of atom probe data.  
The source file (usually ending `.pos`) will be read as 32bit big endian binary numbers with this structure:  
`<X Coord.> <Y Coord.> <Z Coord.> <mass>`

If a correct backup was found it will usually restart from the backup data.

The result is a reciprocal lattice with the log10 of the intensity.  
Exported as a 32bit big endian binary file with the file ending .pos and this format:  
`<X Coord.> <Y Coord.> <Z Coord.> <log10 of intensity>`

For more help start the program over the terminal with `-help`.

## Web
https://github.com/atomprobempie/FourierTransformation

## Credits
- Developer
  - Iceflower S
- Contributors & Help
  - Beha (shacknetisp)
  - B. Gault
  - Bonifarz
  - M. Celik
- IDE
	Code::Blocks

## FAQ
- I got the message `The program can't start because MSVCP140.dll is missing from your computer. Try reinstalling the program to fix this problem`
  - Install Visual C++ Redistributable for Visual Studio 2015
    - https://www.microsoft.com/en-us/download/details.aspx?id=48145
- I got the message `This program can't start because libiomp5md.dll is missing from your computer. Try reinstalling the program to fix this problem`
  - Install the lastest update for Intel® Parallel Studio XE 2016 Composer Edition for C++ Windows* redistributabe package 
    - https://software.intel.com/en-us/articles/redistributables-for-intel-parallel-studio-xe-2016-composer-edition-for-windows
- My screen is freezing.
  - Your CPU has too much to do.
  - You used a to big reciprocal space and run out of memory.

##License
![Image of GPLv3](http://www.gnu.org/graphics/gplv3-127x51.png)

Copyright  ©  2016  Iceflower S

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.  
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.  
You should have received a copy of the GNU General Public License along with this program; if not, see <http://www.gnu.org/licenses/gpl.html>.