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

##License
![Image of GPLv3](http://www.gnu.org/graphics/gplv3-127x51.png)

Copyright  ©  2016  Iceflower S

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.  
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.  
You should have received a copy of the GNU General Public License along with this program; if not, see <http://www.gnu.org/licenses/gpl.html>.