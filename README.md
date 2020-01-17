# goto
A smart cd replace.
# How to install:
1) Run git clone to get the code
2) Edit Makefile, change the install dir to the real target.
3) Run make to build and install goto
4) Run source goto.sh to make sure it work, or directly add the following line to your ~/.bashrc
   source $(bindir)/goto.sh
   $(bindir) is where goto.sh is placed.
   
# How to use:
  ga: add current dir to the goto history.
  
  gd: del current dir from the goto history.
  
  gl: show the goto history.
  
  gc: clear the goto history.
  
  g dir: goto a directory, dir can be any part of the goto history, or a real directory. Use tab can auto complete the result.
  

