#!/usr/bin/perl
#####################################################################
# $Id: batch-build.perl,v 1.2 2002/09/15 14:35:38 bdenney Exp $
#####################################################################
#
# Batch build tool for multiple configurations
#
# switches:
# - show output vs. send it all to nohup.  --nohup
# - serial or parallel.  --parallel
#
# no args: serial, display output
# --nohup: serial, output to nohup.out.  (Need summary.)
# --nohup --parallel: parallel, output to nohup.out
# --parallel: parallel, spawn xterm for each
#

sub usage {
  print <<EOF;
Usage: $0 [--nohup] [--parallel]
--nohup causes the output of all compiles to go into nohup.out
--parallel causes all compiles to be run in parallel

Usage: $0 --clean
--clean erases the build directories and recreates them from scratch

Combinations of nohup and parallel:
no args: serial compile, output goes to stdout
--nohup:  serial compile, output goes into individual nohup.out files
--nohup --parallel: parallel compile, output goes to individual nohup.out files
--parallel: parallel compile, spawn an xterm for each configuration
EOF
}

$DEBUG=0;
$pwd = `pwd`;
chop $pwd;

# create all the configurations that we should test.  The first argument
# is the configuration name, which must be a valid directory name.  To be
# safe, don't put spaces, slashes, ".", or ".." in here.
add_configuration ('normal', 
  '');
add_configuration ('dbg',
  '--enable-debugger');
add_configuration ('smp2',
  '--enable-processors=2');
add_configuration ('smp2-d',
  '--enable-debugger --enable-processors=2');
add_configuration ('64bit',
  '--enable-x86-64');
add_configuration ('64bit-d',
  '--enable-x86-64 --enable-debugger');
add_configuration ('wx',
  '--with-wx');
add_configuration ('wx-d',
  '--with-wx --enable-debugger --disable-readline');
add_configuration ('wx-64bit',
  '--with-wx --enable-x86-64');
add_configuration ('wx-64bit-d',
  '--with-wx --enable-x86-64 --enable-debugger --disable-readline');

my $nohup = 0;
my $parallel = 0;
my $clean = 0;
foreach my $arg (@ARGV) {
  if ($arg eq '--clean') {
    $clean = 1;
  } elsif ($arg eq '--nohup') {
    $nohup = 1;
  } elsif ($arg eq '--parallel') {
    $parallel = 1;
  } else {
    usage(); exit 1;
  }
}

# this script may be run from various directories, and this affects 
# the path to the configure command.  Try to figure out where the configure
# command is found, and set up the build.sh scripts accordingly.  If it
# can't be found, spit out an error now instead of later.
my @configurepath_tries = ("configure", "../configure", "../../configure");
my $configurepath;
foreach my $trypath (@configurepath_tries) {
  if (-x $trypath) {
    print "Found configure at $configurepath.\n" if $DEBUG;
    $configurepath = $trypath;
  }
}
if (!defined $configurepath) {
  print <<EOF;
ERROR I could not locate the configure script.  This script is intended
to be run from the bochs main directory or a subdirectory of it.  Examples:
 1) cd $BOCHS; ./build/batch-build.perl
 2) cd $BOCHS/build; ./batch-build.perl

Here are the places that I tried to find the configure script:
EOF
  foreach (@configurepath_tries) {
    print "  $_\n";
  }
  exit 1;
}

for (my $i=0; $i <= $#config_names; $i++) {
  my $name = "build-$config_names[$i]";
  my $options = $config_opts[$i];
  die if (!defined $name || !defined $options);
  print "Compiling '$name' with opts '$options'\n" if $DEBUG;
  if ($clean) {
    my $rmcmd = "rm -rf $name";
    print "Removing directory $name\n";
    system $rmcmd;
    next;
  }
  if (! -d $name) { mkdir $name, 0755; }
  $maybe_nohup = $nohup? "nohup" : "";
  open (BUILD, ">$name/build.sh");
  print BUILD <<BUILD_EOF;
#!/bin/bash
echo Running the configure script
$maybe_nohup ../$configurepath $options
if test $? != 0; then
  echo Configure failed.
  exit 1
fi
echo Running make
$maybe_nohup make
if test $? != 0; then
  echo Make failed.
  exit 1
fi
BUILD_EOF
  close BUILD;
  chmod 0755, "$name/build.sh";
  $gotodir = "cd $name";
  $startcmd = "$maybe_nohup ./build.sh";
  $header = <<HEADER_EOF;
====================================================================
Configuration name: $name
Directory: $pwd/$name
Config Options: $options
====================================================================
HEADER_EOF
  print $header;

  if ($parallel && !$nohup) {
    # special case for parallel without nohup.  If you're not careful,
    # all output from all compiles will go into the window at once, which
    # is impossible to read.  Also very hard to kill them until they have
    # run their course.  Instead, start each compile in a different xterm!
    # What's even more useful is that after the compile stops it goes into
    # a bash shell so that you can fix things, run the make again, etc.
    #
    # To do this, put the start command in a little shell script called
    # xterm-init.sh.  Start the xterm with "-e xterm-init.sh" so that it
    # runs the script as it starts.

    open (XTI, ">$name/xterm-init.sh");
    print XTI <<XTI_EOF;
#!/bin/bash
cat <<EOF
$header
EOF
$startcmd
bash
exit 0
XTI_EOF
    close XTI;
    chmod 0755, "$name/xterm-init.sh";
    $startcmd = "xterm -title $name -name $name -e ./xterm-init.sh";
  }

  $cmd = "$gotodir && $startcmd";
  $cmd .= "&" if $parallel;
  print "Executing '$cmd'\n" if $DEBUG;
  system $cmd;
}

print "\n"x2;
print "batch-build script is done.\n";
exit 0;

sub add_configuration {
  my ($name, $opts) = @_;
  push @config_names, $name;
  push @config_opts, $opts;
}

