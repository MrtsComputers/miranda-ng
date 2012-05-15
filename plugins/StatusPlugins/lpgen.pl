#!/usr/bin/perl

use POSIX;
use File::Find;
use strict;

my $rootdir = '';
my %hash = ();
my $clines = 0;

my $version = '0.7svn';
my $time = localtime();

if (@ARGV && $ARGV[0] eq "version") {
	shift @ARGV;
	# 1st arg version major i.e. 07 for "0.7"
	if (@ARGV) {
		my $vermaj = shift @ARGV;
		if ($vermaj =~ /^([0-9]*)([0-9])$/) {
			if ($1 eq "") {
				$version = $2;
			} else {
				$version = "$1.$2";
			}
		} else {
			$version = $vermaj;
		}
	}
	# 2nd arg version minor i.e. 1 for "0.7.1"
	if (@ARGV) {
		my $vermin = shift @ARGV;
		if ($vermin =~ /^[0-9]+$/) {
			$version .= ".$vermin";
		} else {
			$version .= $vermin;
		}
	}
	# 3rd arg build number i.e. 36 for "0.7.1 build 36"
	if (@ARGV) {
		my $build = shift @ARGV;
		if ($build =~ /^[0-9]+$/) {
			$version .= " build $build";
		} else {
			$version .= $build;
		}
	}
}

#Language Files
if (!@ARGV) {
	create_langfile(
		'../../miranda',
		'../../miranda/i18n/langpack_english.txt',
		'English (US)', '0809',
		'Miranda IM Development Team',
		'project-info@miranda-im.org',
		'Scriver,avs,chat,clist,clist_nicer,db3x,db3x_mmap,dbrw,advaimg,import,modernb,mwclist,png2dib,srmm,tabsrmm,AimOscar,Gadu-Gadu,IRC,IcqOscarJ,JabberG,MSN,Yahoo');
}
elsif ($ARGV[0] eq "core") {
	create_langfile('../../miranda/src',
		'../../miranda/i18n/core-translation.txt',
		'English (US)', '0809',
		'Miranda IM Development Team',
		'project-info@miranda-im.org');
}
elsif ($ARGV[0] eq "srmm") {
	create_langfile('../../miranda/plugins/SRMM/',
		'../../miranda/plugins/SRMM/Docs/srmm-translation.txt',
		'English (US)', '0809',
		'Miranda IM Development Team',
		'project-info@miranda-im.org');
}
elsif ($ARGV[0] eq "import") {
	create_langfile('../../miranda/plugins/Import/',
		'../../miranda/plugins/Import/docs/import-translation.txt',
		'English (US)', '0809',
		'Miranda IM Development Team',
		'project-info@miranda-im.org');
}
elsif ($ARGV[0] eq "status") {
	create_langfile('./',
		'statusplugins-translation.txt',
		'English (US)', '0809',
		'UnregistereD',
		'unregistered@users.sourceforge.net');
}
else {
	print "Error: Unknown module $ARGV[0]\n";
	exit 1;
}

sub create_langfile {
	$rootdir = shift(@_);
	my $outfile = shift(@_);
	my $lang = shift(@_);
	my $locale = shift(@_);
	my $author = shift(@_);
	my $email = shift(@_);
	my $plugins = @_ ? shift(@_) : '';
	%hash = ();
	my %common = ();
	$clines = 0;
	print "Building language file for $rootdir:\n";
	find({ wanted => \&csearch, preprocess => \&pre_dir }, $rootdir);
	find({ wanted => \&rcsearch, preprocess => \&pre_dir }, $rootdir);
	open(WRITE, "> $outfile") or die;
	print WRITE <<HEADER;
Miranda Language Pack Version 1
Locale: $locale
Authors: $author
Author-email: $email
Last-Modified-Using: Miranda IM $version
Plugins-included: $plugins
; Generated by lpgen on $time
; Translations: $clines

HEADER
	foreach my $trans (keys %hash) {
		if ($hash{$trans} =~ /^\d+$/) {
			$common{$trans} = $hash{$trans};
			delete $hash{$trans};
		}
	}

	my @sorted = sort { (floor($common{$b}/20) == floor($common{$a}/20)) ? ($a cmp $b) : ($common{$b} <=> $common{$a}) } keys %common;
	print WRITE "; Common strings that belong to many files\n;[";
	print WRITE join "]\n;[", @sorted;
	print WRITE "]\n";

	my $file = '';
	foreach my $trans (sort { ($hash{$a} eq $hash{$b}) ? ($a cmp $b) : ($hash{$a} cmp $hash{$b}) } keys %hash) {
		if ($hash{$trans} ne $file) {
			$file = $hash{$trans};
			print WRITE "\n; $file\n";
		}
		print WRITE ";[$trans]\n";
	}

	close(WRITE);
	print "  $outfile is complete ($clines)\n\n";
}

sub pre_dir {
	# avoid parsing some ext SDKs
	my @files = ();
	return @files if (
		$File::Find::name =~/..\/extplugins/ or
		$File::Find::name =~/freeimage\/Source/ or
		$File::Find::name =~/dbrw\/sqlite3/);
	@files = grep { not /^\.\.?$/ } @_;
	return sort @files;
}

sub append_str {
	my $str = shift(@_);
	my $found = shift(@_);
	$str = substr($str, 1, length($str) - 2);
	if (length($str) gt 0 and $str ne "List1" and $str ne "Tree1" and $str =~ /[a-zA-Z]+/g) {
		my $path = $File::Find::name;
		$path =~ s/(\.\.\/)+miranda\///;
		if (!$hash{$str}) {
			$hash{$str} = $path;
			$clines ++;
			return 1;
		} elsif ($hash{$str} ne $path) {
			if ($hash{$str} =~ /^\d+$/) {
				$hash{$str} ++;
			} else {
				$hash{$str} = 1;
			}
		}
	}
	return 0;
}

sub csearch {
	if (-f $_ and ($_ =~ m/\.c(pp)?$/i or $_ =~ m/\.h(pp)?$/i)) {
		my $found = 0;
		my $file = $_;
		print "  Processing $_ ";
		open(READ, "< $_") or return;
		my $all = '';
		while (my $lines = <READ>) {
			$all = $all.$lines;
		}
		close(READ);
		$_ = $all;
		while (/(?:Button_SetIcon_IcoLib|Translate[A-Z]{0,2}|LPGENT?|ICQTranslateUtfStatic)\s*\(\s*(\".*?\")\s*[,\)]/g) {
			$found += append_str($1, $found);
		}
		print "($found)\n";
	}
}

sub rcsearch {
	if ( -f $_ and $_ =~ m/\.rc$/i) {
		my $found = 0;
		my $file = $_;
		print "  Processing $_ ";
		open(READ, "< $_") or return;
		my $all = '';
		while (my $lines = <READ>) {
			$all = $all.$lines;
		}
		close(READ);
		$_ = $all;
		while (/\s*(?:CONTROL|(?:DEF)?PUSHBUTTON|[LRC]TEXT|GROUPBOX|CAPTION|MENUITEM|POPUP)\s*(\".*?\")/g) {
			$found += append_str($1, $found);
		}
		print "($found)\n";
	}
}
