#!/usr/bin/perl
use strict;
use Getopt::Std;

my $errdesc;

# Get program name from executable
my $progname = $0;
$progname = $1 if $progname =~ /([^\/]*)$/;

my %opts;
getopts('s:u:p:xn', \%opts);

my $cmd = shift @ARGV;

# Display usage, if mandatory attibutes not specified
unless ($opts{s} && (defined $cmd)) {
  print STDERR "Usage: $progname -s <server>[:<port>] [-u <user>] [-p <password>] [-x] <cmd>\n",
               "where:  -s  Server URL\n",
               "        -u  User name\n",
               "        -p  User password\n",
               "        -x  XML output instead of iCal\n",
               "        -n  Notify attendees about deletion\n",
               "        <cmd> is command to execute on calendar server\n";
  exit 1;
}
$cmd = lc $cmd;

# Get username from environment, if not specified
unless ($opts{u}) {
  $opts{u} = $ENV{USER};
}

# Read password, if not specified through attribute or environment variable
my $pwd;
if ($opts{p}) {
  $pwd = $opts{p};
} elsif ($ENV{JESCS_OSYNC_PWD}) {
  $pwd = $ENV{JESCS_OSYNC_PWD};
} else {
  print STDERR "Password: ";
  system 'stty -echo';
  $pwd = <STDIN>;
  chomp $pwd;
  system 'stty echo';
  print STDERR "\n";
}

# Create WCAP connection object
my $wa = WCAP::new($opts{s}, $opts{x} ? 'xml' : 'ical');

# Login user to calendar server
if ($wa->login($opts{u}, $pwd) || (not defined $wa->{session_id})) {
  print "Login unsuccessfull\n";
  goto exit_wcaptool_error;;
}

################################################
#####               COMMANDS               #####
################################################

#######################################
## Command GET-ALL
if ($cmd eq 'get-all') {
  # Export calendar (get calendar contents)
  $errdesc = $wa->export;
  if (defined $errdesc) {
    print "ERROR: $errdesc\n";
    goto exit_wcaptool_error;
  } else {
    print &parse_all($wa->{response});
    goto exit_wcaptool_ok;
  }
}

#######################################
## Command GET-EVENTS
elsif ($cmd eq 'get-events') {
  # Export calendar (get calendar contents)
  $errdesc = $wa->export;
  if (defined $errdesc) {
    print "ERROR: $errdesc\n";
    goto exit_wcaptool_error;
  } else {
    print &parse_wcapbuffer($wa->{response}, 'event');
    goto exit_wcaptool_ok;
  }
}

#######################################
## Command GET-TODOS
elsif ($cmd eq 'get-tasks') {
  # Export calendar (get calendar contents)
  $errdesc = $wa->export;
  if (defined $errdesc) {
    print "ERROR: $errdesc\n";
    goto exit_wcaptool_error;
  } else {
    print &parse_wcapbuffer($wa->{response}, 'todo');
    goto exit_wcaptool_ok;
  }
}

#######################################
## Command EXPORT
elsif ($cmd eq 'export') {
  # Export calendar (get calendar contents)
  $errdesc = $wa->export;
  if (defined $errdesc) {
    print "ERROR: $errdesc\n";
    goto exit_wcaptool_error;
  } else {
    print $wa->{response};
    goto exit_wcaptool_ok;
  }
}

#######################################
## Command IMPORT
elsif ($cmd eq 'import') {
  my %arghash;
  # Get filename from argumentlist
  my $fname = shift @ARGV;
  if (not defined $fname) {
    # Filename not specified, read input from STDIN
    $arghash{data} = &read_stdin();
  }
  elsif (not -e $fname) {
    print "ERROR: File '$fname' do not exists\n";
    goto exit_wcaptool_error;
  }
  else {
    $arghash{fname} = $fname;
  }
  # Import calendar (put calendar contents)
  $errdesc = $wa->import(%arghash);
  if (defined $errdesc) {
    print "ERROR: $errdesc\n";
    goto exit_wcaptool_error;
  } else {
    # Get error code from answer
    (my $retval) = $wa->{response} =~ /X-NSCP-WCAP-ERRNO:(\w+)/;
    goto exit_wcaptool_ok unless $retval;
    # Print error code, and exit
    print "ERROR[$retval]: Couldn't import entry\n";
    goto exit_wcaptool_error;
  }
}

#######################################
## Command DELETE
elsif (($cmd eq 'delevent') || ($cmd eq 'deltask')) {
  my %arghash;
  $arghash{entry} = ($cmd eq 'delevent') ? 'event' : 'task';
  # Get entry IDs from argumentlist, or from STDIN
  if (not scalar @ARGV) {
    # Entry IDs not specified, read input from STDIN
    $arghash{ids} = &read_stdin();
  }
  else {
    $arghash{ids} = \@ARGV;
  }
  $arghash{notify} = 1 if $opts{n};
  # Delete calendar contents)
  $errdesc = $wa->delete(%arghash);
  if (defined $errdesc) {
    print "ERROR: $errdesc\n";
    goto exit_wcaptool_error;
  } else {
    # Get error code from answer
    (my $retval) = $wa->{response} =~ /X-NSCP-WCAP-ERRNO:(\w+)/;
    goto exit_wcaptool_ok unless $retval;

    # Some error occured, return status
    my $type = ($cmd eq 'delevent') ? 'EVENT' : 'TODO';
    my $response = $wa->{response};
    my $result = '';
    while ($response =~ /(BEGIN:V$type.*?END:V$type)(.*)$/s) {
      my $block = $1;
      $response = $2;
      (my $uid)    = $block =~ /(UID:[ \w]+)/;
      (my $status) = $block =~ /REQUEST-STATUS:([\.; \w]+)/;
      $result .= ", " if length $result;
      $result .= "$uid($status)";
    }
    print "ERROR: $result\n";
    goto exit_wcaptool_error;
  }
}

#######################################
## Command LIST
elsif ($cmd eq 'list') {
  # List calendars owned, by user
  $errdesc = $wa->do_request('list', "userid=".$wa->{user_id});
  if (defined $errdesc) {
    print "ERROR: $errdesc\n";
    goto exit_wcaptool_error;
  } else {
    print $wa->{response};
  }
}

#######################################
## Unknown command
print STDERR "Unknown command: $cmd\n";



exit_wcaptool_ok:

# Logout user's session
$wa->logout;
exit 0;

exit_wcaptool_error:

# Logout user's session
$wa->logout;
exit 1;

###########################################################################
# Internal functions
###########################################################################

# Read input
sub read_stdin {
  my $buff = '';
  my $byte;
  my $count;
  while (1) {
    $count = sysread STDIN, $byte, 1;
    last unless $count;
    last if $byte eq "\0";
    $buff .= $byte if $byte ne "\r";
  }
  return $buff;
}

# Parse types: event, todo, note
sub parse_wcapbuffer {
  my ($buffer, $type) = @_;
  $type = uc $type;
  my $entitybuf;
  my $out = '';

  # Parse leading buffer VCALENDAR
  chomp(my $header = ($buffer =~ /(BEGIN:VCALENDAR.*?)BEGIN/s) ? $1 : '');
  chomp(my $tail   = ($buffer =~ /.*END:.*?\n(.*END:VCALENDAR)/s) ? $1 : '');

  while ($buffer =~ /(BEGIN:V$type.*?END:V$type)(.*)$/s) {
    $entitybuf = "$header\n$1\n$tail";
    $out .= length($entitybuf)."\n$entitybuf\n";
    $buffer = $2;
  }
  return $out;
}

sub parse_all {
  my $buffer = shift;

  my $out = &parse_wcapbuffer($buffer, 'event');
  $out .= &parse_wcapbuffer($buffer, 'todo');
  $out .= &parse_wcapbuffer($buffer, 'note');

  return $out;
}

###########################################################################
# WCAP connection object
###########################################################################

package WCAP;
use LWP::UserAgent;		# 'Fake' browser
use LWP::ConnCache;

# Globals
use constant GMT_DELTA => 11;   # Don't forget to change for daylight savings

sub new {
  my ($server_url, $format, $user, $password) = @_;
  my %response_formats = ( ical       => 'text/calendar',
	                   xml        => 'text/xml',
                           #javascript => 'text/js',
                         );
  my $out_format = ((defined $format) && (defined $response_formats{$format})) ? $response_formats{$format} : $response_formats{ical};

  my $ua = LWP::UserAgent->new;
  $ua->agent('OpenSync_JESCS/0.1');
  # Enable connection cache (use same socket for all connections - speeds up connection)
  $ua->conn_cache(LWP::ConnCache->new());

  my $self = { # Communication attributes
               url           => $server_url,
               format        => $out_format,
               formats       => \%response_formats,
               # Session attributes
               user_id       => undef,
               calendar_id   => undef,
               session_id    => undef,
               response      => undef,
               # User Agent
               ua            => $ua
             };
  bless $self, 'WCAP';

  # Login user, if specified
  if ((defined $user) && (defined $password)) {
    $self->login($user, $password);
  }

  return $self;
}

# Log in to the calendar server
# Returns status error code or 
#                0 on success or
#                -1 on wrong url
sub login {
  my ($self, $user, $password) = @_;
  my $errno = -1;
  # Compose login URL
  my $req_url = "http://".$self->{url}."/login.wcap?user=$user&password=$password&fmt-out=".$self->{formats}->{ical};
  # Send login request
  my $response = $self->{ua}->get($req_url);
  if ($response->is_success) {
    # Login response
    my $content = $response->content;
    $self->{response} = $content;
    # Fetch IDs
    ($self->{user_id})     = $content =~ /WCAP-USER-ID:(\w+)/;
    ($self->{calendar_id}) = $content =~ /WCAP-CALENDAR-ID:(\w+)/;
    ($self->{session_id})  = $content =~ /WCAP-SESSION-ID:(\w+)/;
    ($errno) = $content =~ /WCAP-ERRNO:(\d+)/;
  } else {
    $self->{response} = $response->status_line;
  }
  return $errno;
}

# Log out session from the calendar server
# Returns status error code or 
#                0 on success or
#                -1 on wrong url
sub logout {
  my $self = shift;
  # Return, if session not logged in
  return -1 unless defined $self->{session_id};
  my $errno = -1;
  # Compose logout URL
  my $req_url = "http://".$self->{url}."/logout.wcap?id=".$self->{session_id}."&fmt-out=".$self->{formats}->{ical};
  # Send logout request
  my $response = $self->{ua}->get($req_url);
  if ($response->is_success) {
    # Logout response
    my $content = $response->content;
    $self->{response} = $content;
    # Fetch state
    ($errno) = $content =~ /WCAP-ERRNO:(\d+)/;
    unless ($errno) {
      $self->{user_id}     = undef;
      $self->{calendar_id} = undef;
      $self->{session_id}  = undef;
    }
  } else {
    $self->{response} = $response->status_line;
  }
  return $errno;
}

# Import calendar
# Arguments:  fname     => <name>       name of file containing data to import (used only, if 'data' not specified)
#             data      => <buffer>     data to import
#             calendars => [ <cals> ]   calendar ID's
#             dtstart   => <dtstart>    start date
#             dtend     => <dtstop>     stop date
# Returns error description on error
#         or undef on success (response in $object->{response})
sub import {
  my ($self, %data) = @_;
  # Return if no file name specified
  return "No data to import" unless (defined $data{fname}) || (defined $data{data});
  # Return, if session not logged in
  return "No logged in session present" unless defined $self->{session_id};
  # Correct calendar arrayref parameter
  if (not defined $data{calendars}) {
    $data{calendars} = [ $self->{calendar_id} ];
  } elsif (ref($data{calendars}) ne 'ARRAY') {
    $data{calendars} = [ $data{calendars} ];
  }
  # Read calendar buffer from file, if no data specified
  unless (defined $data{data}) {
    if (not open FD, "<$data{fname}") {
      return "Cannot read file $data{fname}\n";
    }
    my @lines = <FD>;
    close FD;
    foreach my $line (@lines) {
      $line =~ s/[\r\n]$//g;
    }
    $data{data} = join("\n",@lines);
  } else {
    $data{data} =~ s/\r//sg;
  }
  # Fetch basename of file
  if (defined $data{fname}) {
    ($data{fname}) = $data{fname} =~ /([^\/]+)$/;
  } else {
    $data{fname} = 'ical.ics';
  }
  # Compose requesting URL
  my $import_url = "http://".$self->{url}."/import.wcap?id=".$self->{session_id}.         # Session ID
                   "&calid=".(join(';',@{$data{calendars}})).                             # Calendar(s) ID
                   "&content-in=".$self->{format}.                                        # Contents format
                   "&dtstart=".((defined $data{dtstart}) ? $data{dtstart} : 0).           # start DateTime
                   "&dtend=".((defined $data{dtend}) ? $data{dtend} : 0);                 # end DateTime
  # Send import request
  my $response = $self->{ua}->post($import_url,
                                   Content_Type   => 'form-data',
                                   Content        => [ Upload => [ undef,                             # filename (undef = contents in buffer)
                                                                   $data{fname},                      # base filename
                                                                   Content_Type   => $self->{format},	    # contents format
                                                                   Content_Length => length($data{data}),   # contents length
                                                                   Content        => $data{data}            # buffer
                                                                 ]
                                                     ]
                                  );
  return $response->status_line unless $response->is_success;

  # Import response
  my $content = $response->content;
  $self->{response} = $content;
  return undef;
}

# Export calendar
# Arguments:  calendars => [ <cals> ]   calendar ID's
#             dtstart   => <dtstart>    start date
#             dtend     => <dtstop>     stop date
# Returns: undef on error
#          Response buffer on success
sub export {
  my ($self, %data) = @_;
  # Return, if session not logged in
  return "No logged in session present" unless defined $self->{session_id};
  # Correct calendar arrayref parameter
  if (not defined $data{calendars}) {
    $data{calendars} = [ $self->{calendar_id} ];
  } elsif (ref($data{calendars}) ne 'ARRAY') {
    $data{calendars} = [ $data{calendars} ];
  }
  # Compose requesting URL
  my $export_url = "http://".$self->{url}."/export.wcap?id=".$self->{session_id}.         # Session ID
                   "&calid=".(join(';',@{$data{calendars}})).                             # Calendar(s) ID
                   "&content-out=".$self->{format}.                                       # Contents format
                   "&dtstart=".((defined $data{dtstart}) ? $data{dtstart} : 0).           # start DateTime
                   "&dtend=".((defined $data{dtend}) ? $data{dtend} : 0);                 # end DateTime
  # Send export request
  my $response = $self->{ua}->post($export_url,
                                   Content_Type   => 'form-data',
                                   Content        => [ Download => [ undef, '',
                                                                     Accept          => 'image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, image/png, */*',
                                                                     Accept_Encoding => 'deflate,gzip',
                                                                     Accept_Language => 'en',
                                                                     Accept_Charset  => 'iso-8859-1,iso-8859-2,*,utf-8'
                                                                   ]
                                                     ]
                                  );
  return $response->status_line unless $response->is_success;

  # Export response
  my $content = $response->content;
  $self->{response} = $content;
  return undef;
}

# Delete entry from calendar
# Arguments:  entry     => <entry>                  'event' or 'task'
#             ids       => [ <id1>, <id2>, ... ]    entry/todo ID
#             calendars => [ <cal1>, <cal2>, ... ]  calendar ID's
#             notify    => notify attendees about deletion, if this key present
# Returns error description on error
#         or undef on success (response in $object->{response})
sub delete {
  my ($self, %data) = @_;
  my $command;
  # Return if no entry specified
  return "No entry type specified" unless defined $data{entry};
  if (lc($data{entry}) eq 'event') {
    $command = 'deleteevents_by_id';
  } elsif (lc($data{entry}) eq 'task') {
    $command = 'deletetodos_by_id';
  } else {
    return "Unknown entry type '$data{entry}'";
  }
  # Return if no entry ID specified
  return "No data to delete" unless defined $data{ids};
  # Return, if session not logged in
  return "No logged in session present" unless defined $self->{session_id};
  # Correct calendar arrayref parameter
  if (not defined $data{calendars}) {
    $data{calendars} = [ $self->{calendar_id} ];
  } elsif (ref($data{calendars}) ne 'ARRAY') {
    $data{calendars} = [ $data{calendars} ];
  }
  # Correct entry IDs parameter
  if (ref($data{ids}) ne 'ARRAY') {
    $data{ids} = [ $data{ids} ];
  }

  # Make delete request
  my $idcount = scalar @{$data{ids}};
  chop(my $rids = "0;" x $idcount);   # Recurrence identifiers (all 0, we are deleting entry with all of it's recurrences)
  chop(my $mods = "4;" x $idcount);   # 4 = delete entry with all of it's recurrences
  return $self->do_request($command, "calid=".(join(';',@{$data{calendars}})),
                                     "uid=".(join(';',@{$data{ids}})),
                                     "rid=$rids",
                                     "mod=$mods",
                                     "notify=".($data{notify} ? 1 : 0));
}

# Send a WCAP request
# Returns error description on error
#         or undef on success (response in $object->{response})
#
# NOTE: There is a limit to the number of characters that may be passed
# in for each parameter. The limit per parameter is 1000 characters.
# (p80 Sun ONE Calendar Server Programmer's Manual, August 2002)
#
# Note: Commands beginning with '&' are anonymous
sub do_request {
  my ($self, $command, @args) = @_;
  # Check, if command is anonymous, or not
  my $anonymous = ($command =~ /^\&/) ? 1 : 0;
  # Remove leading '&' from command if present
  $command =~ s/^\&//g;
  # Check session and command
  unless ($anonymous || (defined $self->{session_id})) {
    return "No logged in session present";
  }
  unless (defined $command) {
    return "No command specified";
  }
  # Do the request
  my $req_url = "http://".$self->{url}."/${command}.wcap?fmt-out=".$self->{format};
  $req_url .= "&id=".$self->{session_id} if defined $self->{session_id};
  if (@args) {
    $req_url .= '&'.join('&', @args);
  }
  my $response = $self->{ua}->get($req_url);
  return $response->status_line unless $response->is_success;

  $self->{response} = $response->content;
  return undef;
}

