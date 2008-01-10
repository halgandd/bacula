#!/usr/bin/perl -w

=head1 LICENSE

   Bweb - A Bacula web interface
   Bacula� - The Network Backup Solution

   Copyright (C) 2000-2006 Free Software Foundation Europe e.V.

   The main author of Bweb is Eric Bollengier.
   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.

   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation plus additions
   that are listed in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula� is a registered trademark of John Walker.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zurich,
   Switzerland, email:ftf@fsfeurope.org.

=cut

use strict;
use GTime;
use Getopt::Long ;
use Bweb;
use Time::ParseDate qw/parsedate/;
use POSIX qw/strftime/;
use CGI;

my $conf = new Bweb::Config(config_file => $Bweb::config_file);
$conf->load();

my $bweb = new Bweb(info => $conf);
my $arg = $bweb->get_form(qw/qiso_begin qiso_end qusage qpool qnojob
                             jclient_groups db_client_groups qclient_groups/);

if (!$arg->{qiso_begin}) {
   $arg->{qiso_begin} = strftime('\'%F %H:%M:00\'', localtime(time - 60*60*12));
   $arg->{qiso_end} = strftime('\'%F %H:%M:00\'', localtime(time));
}
use Digest::MD5 qw(md5_hex);
my $md5_rep = md5_hex("$arg->{qiso_begin}:$arg->{qiso_end}:$arg->{qusage}:" . 
		      "$arg->{jclient_groups}:$arg->{qpool};$arg->{qnojob}") ;

print CGI::header('text/html');
$bweb->display_begin();

if ($arg->{qiso_begin} && -f "$conf->{fv_write_path}/$md5_rep.png") {
    $arg->{result} = "/bweb/fv/$md5_rep.png";
    $bweb->display($arg, 'btime.tpl');
    
    $bweb->display_end();
    exit 0;
}

my $top = new GTime(
		    debug => $conf->{debug},
		    xmarge => 200,
		    type => {
                        spool =>  1,
                        despool =>  2,
                        commit => 4,
			waiting => 3,
                        init => 5,
                    }) ;
my $sec = $bweb->{sql}->{STARTTIME_SEC};
$sec =~ s/Job.StartTime/Log.Time/;
my $reg = $bweb->{sql}->{MATCH};


my %regs = (
	    end_job => ': Bacula',
	    start_job => ': Start Backup',
	    data_despool_time => ': Despooling elapsed time',
	    attr_despool_time => ': Sending spooled attrs',
	    get_drive => ': Using Device',
	    start_spool => ': Spooling',
	    end_spool => ': User specified spool',
	    end_spool2 => ': Committing spooled data to',
	    );


my %regs_fr = (
	    end_job => ': Bacula',
	    start_job => ': D.marrage du ',
	    data_despool_time => ': Temps du tran',
	    attr_despool_time => ': Transfert des attributs',
	    get_drive => ': Using Device',
	    start_spool => ': Spooling',
	    end_spool => ': Taille du spool',
	    end_spool2 => ': Transfert des donn',
	    );

my $filter = join(" OR ", 
		  map { "Log.LogText $bweb->{sql}->{MATCH} '$_'" } values %regs);

my $query = "
SELECT " . $bweb->dbh_strcat('Job.Name', "'_'", 'Job.Level') . ",
       $sec,
       substring(Log.LogText from 8 for 70),
       Job.JobId,
       Pool.Name

FROM  Log INNER JOIN Job USING (JobId) JOIN Pool USING (PoolId)
" . ($arg->{jclient_groups}?
"     JOIN Client USING (ClientId) 
      JOIN client_group_member ON (Client.ClientId = client_group_member.clientid) 
      JOIN client_group USING (client_group_id)
      WHERE client_group_name IN ($arg->{jclient_groups})
        AND
":'WHERE') .
"
      Job.StartTime > $arg->{qiso_begin}
  AND Job.StartTime < $arg->{qiso_end}
  AND ( $filter )
  AND Job.Type = 'B'
ORDER BY Job.JobId,Log.LogId,Log.Time  
";


print STDERR $query if ($conf->{debug});
my $all = $bweb->dbh_selectall_arrayref($query);

my $lastid = 0;
my $lastspool = 0;
my $last_name;
my $data = [];
my $write = {};
my $pool = {};
my $drive = "";
my $end;
my $begin;
foreach my $elt (@$all)
{
    if ($lastid && $lastid ne $elt->[3]) {
	if (!$arg->{qnojob}) {
	    $top->add_job(label => $last_name,
			  data => $data);
	}
	$data = [];
	$lastspool=0;
    }

    if ($elt->[2] =~ /$regs{end_job}/) {
	push @$data, {
	    type  => "commit",
	    begin => $begin,
	    end   => $elt->[1],
	};

#    } elsif ($elt->[2] =~ /$regs{attr_despool_time}/) {
#
#	push @$data, {
#	    l => $elt->[2],
#	    type  => "waiting",
#	    begin => $begin,
#	    end   => $elt->[1],
#	};
#
    } elsif ($elt->[2] =~ /$regs{get_drive} "([\w\d]+)"/) {
	$drive = $1;

    } elsif ($elt->[2] =~ /$regs{data_despool_time}.+? = (\d+):(\d+):(\d+)/) {
	# on connait le temps de despool
	my $t = $1*60*60+ $2*60 + $3;

	if ($t > 10) {		# en dessous de 10s on affiche pas
	    push @$data, {	# temps d'attente du drive
#		l => $elt->[2],
		type  => "waiting",
		begin => $begin,
		end   => parsedate($elt->[1]) - $t,
	    };

	    push @$data, {
#		l => $elt->[2],
		type  => "despool",
		begin => parsedate($elt->[1]) - $t,
		end   => $elt->[1],
	    };

	    push @{$write->{$drive}}, {	# display only write time
		type  => "despool",
		begin => parsedate($elt->[1]) - $t,
		end   => $elt->[1],
	    };

	    push @{$pool->{"$drive: $elt->[4]"}}, {
		type  => "despool",
		begin => parsedate($elt->[1]) - $t,
		end   => $elt->[1],
	    };
	} else {
	    push @$data, {
#		t => $t,
#		l => $elt->[2],
		type  => "waiting",
		begin => $begin,
		end   => $elt->[1],
	    };
	}
	
    } elsif ($elt->[2] =~ /$regs{start_spool}/) {

	if (!$lastspool) {
	    push @$data, {
		type  => "init",
		begin => $begin,
		end   => $elt->[1],
	    };	
	}

	$lastspool = 1;

    } elsif ($elt->[2] =~ /($regs{end_spool}|$regs{end_spool2})/) {
	push @$data, {
#	    l => $elt->[2],
	    type  => "spool",
	    begin => $begin,
	    end   => $elt->[1],
	};

    } elsif ($elt->[2] =~ /$regs{start_job}/) {
	1;
    } else {
	next;
    }

    $end = $begin;
    $begin = $elt->[1];
    $last_name = $elt->[0];
    $lastid = $elt->[3];
}

if (!$arg->{qnojob} && $last_name) {
    $top->add_job(label => $last_name,
		  data => $data);
}
if ($arg->{qpool}) {
    foreach my $d (sort keys %$pool) {
	$top->add_job(label => $d,
		      data => $pool->{$d});
    }
}

if ($arg->{qusage}) {
    foreach my $d (sort keys %$write) {
	$top->add_job(label => "drive $d",
		      data => $write->{$d});
    }
}

#print STDERR Data::Dumper::Dumper($top);

$top->finalize();

open(FP, ">$conf->{fv_write_path}/$md5_rep.png");
binmode FP;
# Convert the image to PNG and print it on standard output
print FP $GTime::gd->png;
close(FP);

$arg->{result} = "/bweb/fv/$md5_rep.png";
$bweb->display( $arg, 'btime.tpl');
$bweb->display_end();
