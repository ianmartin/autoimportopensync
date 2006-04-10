file /usr/bin/msynctool
set args  --sync evo2-syncml-sw --filter-objtype contact --filter-objtype todo --conflict i
shell rm /tmp/sync.log/*
set environment OSYNC_TRACE = /tmp/sync.log
directory /usr/src/debug/syncml-client-plugin-0.01/src
directory /usr/src/debug/multisync-cli-0.18/tools
