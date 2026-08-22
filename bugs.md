- this bug was found by cppcheck ... it is likely a valid bug.
regif.cpp:215:15: style: Variable 'eqptr' is assigned a value that is never used. [unreadVariable]
         eqptr++ ;

This is in function `WriteRegifProfileString()` ;  
I wonder if that function was ever actually used??  
I do not think that variable does what you think it does... 

This bug is also present in `regif.cpp` in project `binclock`,  
though in that project, `regif` has been superceded by `config.cpp`,  
which is much easier to use.

***

"select console launch directory" does not let me select directory...
Well, I have to select an unwanted file in the target directory; *then* it works...

***
put tooltips on the controls?

***
an explicit `close` button, as well as the taskbar X, would be very intuitive...


