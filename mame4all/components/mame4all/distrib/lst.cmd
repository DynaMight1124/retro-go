.\mame4all\mame -gamelistheader 
move .\mame4all\stdout.txt gamelist.txt
.\mame4all\mame -gamelist 
copy gamelist.txt+.\mame4all\stdout.txt 
.\mame4all\mame -gamelistfooter 
copy gamelist.txt+.\mame4all\stdout.txt 
.\mame4all\mame -listcrc
move .\mame4all\stdout.txt gameromlist.txt
