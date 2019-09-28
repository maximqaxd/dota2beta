@echo off
python vpc_wrapper.py /deferred +game /mksln Game_Deferred-2019.sln /vs2019 /full_rebuild %0 %1 %2 %3 %4 %5 %6 %7 %8 %9 
python vpc_wrapper.py /template +game /mksln Game_Template-2019.sln /vs2019 /full_rebuild %0 %1 %2 %3 %4 %5 %6 %7 %8 %9 
python vpc_wrapper.py /portal_template +game /mksln Game_Portal_Template-2019.sln /vs2019 /full_rebuild %0 %1 %2 %3 %4 %5 %6 %7 %8 %9 
python vpc_wrapper.py /hl2_template +game /mksln Game_HL2_Template-2019.sln /vs2019 /full_rebuild %0 %1 %2 %3 %4 %5 %6 %7 %8 %9 
python vpc_wrapper.py /vectronic +game /mksln Game_Vectronic-2019.sln /vs2019 /full_rebuild %0 %1 %2 %3 %4 %5 %6 %7 %8 %9 