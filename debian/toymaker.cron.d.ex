#
# Regular cron jobs for the toymaker package.
#
0 4	* * *	root	[ -x /usr/bin/toymaker_maintenance ] && /usr/bin/toymaker_maintenance
