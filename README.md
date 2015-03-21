qgmailnotifier is a portable Qt4/Qt5 based GMail notifier, which is designed to provide all of the functionality that the official Windows notifier has, and more.

![Example Image](http://www.codef00.com/img/qgmailnotifier1.png)

The only requirement for building qgmailnotifier is Qt4/Qt5.

Installation is simple. To build, simply type:

  $ qmake
  $ make

On some distributions, the Qt4 qmake is called `qmake-qt4`, so on these you 
would type:

  $ qmake-qt4
  $ make

Finally, to install into "/usr/bin":

  $ make INSTALL_ROOT=/usr/ install

Note:

Your GMail user name and password are currently stored in plain text in the
config file. The only real solution to this is keyring support (like kwallet)
which is being worked on.

Enjoy!
