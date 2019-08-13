<?php

//wget http://files.virtualizor.com/install.sh; chmod 0755 install.sh; ./install.sh email=alons@softaculous.com noos=1

$boot_file = '/etc/grub.conf';

if(!file_exists($boot_file) && file_exists('/boot/grub/menu.lst')){
	$boot_file = '/boot/grub/menu.lst';
}

// Set the environment variables for the binaries
if(strtoupper(substr(PHP_OS, 0, 3)) != 'WIN'){
	putenv('PATH=/usr/kerberos/sbin:/usr/kerberos/bin:/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin:/root/bin:/usr/local/emps/bin:/usr/local/emps/sbin');
}

//////////////////////////
// Some INSTALL Functions
//////////////////////////

function install_handle_storage($virt){

	global $args, $globals, $ostemplates, $oses, $distro;
	
	// Is there an LV ?
	if(!empty($globals['lv'])){
		
		// Search the LV Group
		exec($globals['com']['vgdisplay'].' '.$globals['lv'].' >> /dev/null 2>&1', $vgout, $vgret);
		
		if($vgret != 0){
			echo "	ERROR : Logical Volume Group NOT FOUND \n Please specify the correct Logical Volume Group and then RE-Run this installer \n";
			shell_exec('echo "ERROR : Logical Volume Group NOT FOUND \n Please specify the correct Logical Volume Group and then RE-Run this installer" >> '.$args['log'].' 2>&1');
			return false;
		}
		
		// DO YOU have enough space IN IT
		exec($globals['com']['vgdisplay'].' -C --nosuffix -o Free --units K "'.$globals['lv'].'" 2>/dev/null', $volgr);
		
		if(empty($volgr[1])){
			echo "	ERROR : Logical Volume Group - '".$globals['lv']."' is FULL\n Please specify a Logical Volume Group which has enough space to create VPS(s) \n";
			shell_exec('echo "ERROR : Logical Volume Group - "'.$globals['lv'].'" is FULL\n Please specify a Logical Volume Group which has enough space to create VPS(s)" >> '.$args['log'].' 2>&1');
			return false;
		}
	
	// No storage	
	}else{
	
		echo "		- You have not defined any storage ! Please add a Storage once you visit the Admin Panel.\n";
		shell_exec('echo "		- You have not defined any storage ! Please add a Storage once you visit the Admin Panel." >> '.$args['log'].' 2>&1');
		
	}
	
	return true;
	
}

// Installs Master only
function install_master(){

	global $args, $globals, $ostemplates, $oses, $distro;
	
	echo "	This is MASTER ! No Kernel to install ... Continuing \n";
	shell_exec('echo "This is MASTER ! No Kernel to install ... Continuing" >> '.$args['log'].' 2>&1');
	
	return true;
	
}

// Installs OpenVZ
function install_openvz(){

	global $args, $globals, $ostemplates, $oses, $distro;
	
	if(!isset($distro)){	
		$distro	 = get_distro();		
	}
	
	if(file_exists('/etc/vz/vz.conf')){
	
		// Install the packages even if OpenVZ is installed
		// Fix for SolusVM Imported Servers
		_package_install("vzdump=1.2-7", $args['log']);
		
		echo "	OpenVZ already installed ... Continuing \n";
		shell_exec('echo "OpenVZ already installed ... Continuing" >> '.$args['log'].' 2>&1');
	}else{
	
		// Add the REPO file
		if($distro != 'ubuntu'){
			shell_exec('wget -O /etc/yum.repos.d/openvz.repo http://download.openvz.org/openvz.repo >> '.$args['log'].' 2>&1');			
		}else{
			shell_exec('echo "deb http://mirror.softaculous.com/virtualizor/debian wheezy main" > /etc/apt/sources.list.d/virtualizor.list 2>&1');
			shell_exec('echo "deb http://download.openvz.org/debian wheezy main" > /etc/apt/sources.list.d/openvz-rhel6.list 2>&1');
			
			exec('mkdir -p /tmp/keys 2>/dev/null');
			shell_exec('wget -O /tmp/keys/virtualizor.key http://mirror.softaculous.com/virtualizor/debian/virtualizor.key >> '.$args['log'].' 2>&1');
			
			shell_exec('wget -O /tmp/keys/archive.key http://ftp.openvz.org/debian/archive.key >> '.$args['log'].' 2>&1');
			
			shell_exec('apt-key add /tmp/keys/virtualizor.key');
			shell_exec('apt-key add /tmp/keys/archive.key');
			shell_exec('rm -rf /tmp/keys/');
			shell_exec('apt-get -y update');
		}
		
		if($distro != 'ubuntu'){
			$release = shell_exec('cat /etc/redhat-release');
			
			// CentOS 5.x need the old one
			if(preg_match('/release 5/is', $release)){
				shell_exec('mv /etc/yum.repos.d/openvz.repo /etc/yum.repos.d/openvz.repo_');
				shell_exec('cat /etc/yum.repos.d/openvz.repo_ | sed \'20s/enabled=1/enabled=0/\' | sed \'58s/enabled=0/enabled=1/\' > /etc/yum.repos.d/openvz.repo');
				shell_exec('rm -rf /etc/yum.repos.d/openvz.repo_');
			}
		}
		
		sleep(1);
		
		$arch = `arch`;
		$arch = trim($arch);
		
		if($distro != 'ubuntu'){
		
			if($arch === 'x86_64'){
				$list = array("vzctl.$(uname -i)", "vzquota.$(uname -i)", "ovzkernel.$(uname -i)");
			}else{
				$list = array("vzctl", "vzquota", "ovzkernel-PAE");
			}
			
			// Additional Common RPMs
			$list[] = 'vzdump';
			
			
			// ploop only available for Centos 6 > 064stab
			$release = file_get_contents('/etc/redhat-release');
			if(preg_match('/release 6/is', $release)){
				$list[] = 'ploop-lib';
			}
			
		}else{
			$list = array("vzctl", "vzquota", "ploop", "vzstats", "cstream", "liblockfile-simple-perl", "perl", "vzdump=1.2-7");
			
			if($arch === 'x86_64'){
				$list[] = 'linux-image-openvz-amd64';
			}else{
				$list[] = 'linux-image-openvz-686';
			}
		}
		
		// Install the packages
		_package_install($list, $args['log']);
		
		// Was it installed ?
		if(!file_exists('/etc/vz/vz.conf')){
			echo "	Errors while installing OpenVZ \n";
			return false;
		}
		
		// Save the vz.conf
		save_web_file('http://files.virtualizor.com/vz.conf', '/etc/vz/vz.conf');
		
		// The IP Conntrack stuff
		shell_exec('echo options ip_conntrack ip_conntrack_enable_ve0=1 >> /etc/modprobe.conf');
		
		// The sysctl.conf
		save_web_file('http://files.virtualizor.com/sysctl.conf', '/etc/sysctl.conf');
		
		// Flushes IPv6 tables
		vexec('/sbin/ip6tables -F');
		vexec('/sbin/service ip6tables save');
		
		// Edit the GRUB
		if($distro != "ubuntu"){
			$menu = implode("", file($GLOBALS['boot_file']));
			$newmenu = preg_replace('/default=(\d*)/is', 'default=0', $menu);
			writefile($GLOBALS['boot_file'], $newmenu, 1);			
			yum_exclude();
		}else{
			exec('lsb_release -r', $out1, $ret1);
			if(preg_match('/14.04/is', $out1[0])){
				$default_val = 'default="1>2"';
			}else{
				$default_val = 'default="2"';
			}
			
			$menu = implode("", file('/boot/grub/grub.cfg'));
			$newmenu = preg_replace('/default=\"(\d*)\"/is', $default_val, $menu);
			writefile('/boot/grub/grub.cfg', $newmenu, 1);
		}
	}
	
	// Ensure the OS Templates folder is there
	@mkosdir();
	
	$osid = $GLOBALS['latest_os']['openvz'];
	
	// Save a OS Template
	$res = makequery("REPLACE INTO `os`
						SET osid = '".$osid."'");
	
	// Download if allowed
	if(empty($args['noos'])){
		shell_exec('wget -O /vz/template/cache/'.$oses[$osid]['filename'].' '.$oses[$osid]['url'].' >> '.$args['log'].' 2>&1');
	}
	
	// TC add basic root qdisc with id 10 to the venet0
	//shell_exec('tc qdisc add dev venet0 root handle 10: cbq bandwidth 100Mbit avpkt 1000');
	
	// Make sure the module loading code is there
	// RedHat Style
	if(file_exists('/etc/sysconfig/modules')){
		
		file_put_contents('/etc/sysconfig/modules/vzvirtualizor.modules', '#!/bin/sh
modprobe tun
modprobe ppp-compress-18
modprobe ppp_mppe
modprobe ppp_deflate
modprobe ppp_async
modprobe pppoatm
modprobe ppp_generic');

		chmod('/etc/sysconfig/modules/vzvirtualizor.modules', 0755);
		shell_exec('/etc/sysconfig/modules/vzvirtualizor.modules >> '.$args['log'].' 2>&1');
		
	// Debian Style
	}elseif(file_exists('/etc/modules')){
		
		$etc_modules = file_get_contents('/etc/modules');
		
		// Is there an entry already
		if(substr_count($etc_modules, 'ppp') < 1){
			
			$etc_modules = $etc_modules.'
tun
ppp-compress-18
ppp_mppe
ppp_deflate
ppp_async
pppoatm
ppp_generic	
';
			
			file_put_contents('/etc/modules', $etc_modules);
			
		}
		
	}
	
	return true;
	
}

// Function to install XEN
function install_xen(){
	
	global $args, $globals, $ostemplates, $oses, $distro;
	
	if(!isset($distro)){	
		$distro = get_distro();		
	}
	
	// Handle Storage
	if(!install_handle_storage('xen')){
		return false;	
	}
		
	$release = file_get_contents('/etc/redhat-release');
	
	// centos-release-xen will be installed only in CentOS 6
	// XEN Repo to install on CentOS 6 OR 7
	if(preg_match('/release (6|7)/is', $release)){
		$centos6 = 1;
		$yum_update = 1;
	}
	
	
	// Is it installed
	if(is_dir('/etc/xen') && empty($args['import'])){
		echo "	Xen already installed ... Continuing \n";
		shell_exec('echo "Xen already installed ... Continuing" >> '.$args['log'].' 2>&1');
	}else{
	
		
		if($distro == 'ubuntu'){
		
			$arch = exec('arch');
			if($arch == 'i686'){
				$xen_mod = 'xen-hypervisor-*-i386';
			}else{
				$xen_mod = 'xen-hypervisor';
			}
			
			// For skipping libguestfs-tools interactive mode
			putenv('DEBIAN_FRONTEND=noninteractive');
			
			// Generate the list of packages to be installed for Ubuntu
			$list = array("libvirt-bin", "bridge-utils", "ntfs-3g", "sysv-rc-conf", $xen_mod, "dhcp3-server", "libvncserver0", "python-numpy", "chntpw", "qemu-img", "libguestfs-tools", "guestmount", "ebtables");
		}else{
		
			// First install libvirt with default repo
			_package_install('libvirt', $args['log']);
			
			// Generate the list of packages to be installed for CentOS
			$list = array("centos-release-xen", "ntfsprogs", "ntfs-3g", "dhcp", "libvncserver", "numpy", "chntpw", "qemu-img", "guestfish", "libguestfs-tools", "libguestfs-tools-c", "ebtables", "libguestfs-winsupport", "net-tools");
			
		}
		
		// Install the packages
		_package_install($list, $args['log']);
		
		if(!empty($yum_update)){
			@shell_exec('yum -y update >> '.$args['log'].' 2>&1');
		}
		
		// As per teh doc kernel-xen needs to be installed after xen package
		if($distro != 'ubuntu'){
			_package_install("xen", $args['log']);
		}
		
		// Enable DHCP
		_init_config(($distro == 'ubuntu' ? 'isc-dhcp-server' : 'dhcpd'), 19);
		
		//_init_config('xend');
		
		// Was it installed ?
		if(!is_dir('/etc/xen')){
			echo "	Errors while installing Xen \n";
			return false;
		}
		
		$default = -1;
		
		// Edit the GRUB to the Xen Kernel - Not applies to UBUNTU
		/*if($distro != 'ubuntu'){
		
			$menu = file($GLOBALS['boot_file']);
			foreach($menu as $k => $v){
				$v = trim($v);
				if(empty($v{0}) || @$v{0} == '#'){
					continue;
				}
				
				// Is it a title
				if(preg_match('/^title(.*?)/is', $v)){
					$default++;
					
					// Is it a XEN kernel title ?
					if(preg_match('/^title(.*?)xen(.*?)/is', $v)){
					
						$found = 1;
					
						if(preg_match('/kernel(\s*?)(.*?)xen/i', $menu[$k + 2])){
							
							$newline = '/'. preg_replace('/\//', '\/',  $menu[$k + 2]).'/';						
							
							if(preg_match('/dom0_mem=(.*?)/i', $menu[$k + 2])){
								// Change previous value 
								preg_replace('/dom0_mem("|\=)(.*?)M/ies', '$rest = trim(\'$2\')', $menu[$k + 2]);
								$finalline = str_replace($rest, "1024", $menu[$k + 2]);
							} else{
								// Add Dom0_mem=1024M inside file
								$finalline = rtrim($menu[$k + 2])." dom0_mem=1024M\n";
							}
						} 
					
						break;
					}
				}
				
			}
			
			if($default >= 0 && !empty($found)){
				$newmenu = preg_replace('/default=(\d*)/is', 'default='.$default, implode('', $menu));
			}
		
			// Add RAM settings
			if(!empty($finalline)){
				$buff = preg_replace($newline, $finalline, $newmenu);
				writefile($GLOBALS['boot_file'], $buff, 1);
			}
			
			// We need to enable network_bridge for XM
			$xend_config = file_get_contents('/etc/xen/xend-config.sxp');
			$xend_config = str_replace('# (network-script network-bridge)', ' (network-script network-bridge)', $xend_config);
			$xend_config = str_replace('#(network-script network-bridge)', '(network-script network-bridge)', $xend_config);
			file_put_contents('/etc/xen/xend-config.sxp', $xend_config);
						
		}*/
		
		// Download kernel file for Ubuntu from centos
		// Set the image directory
		if($distro == 'ubuntu' || !empty($centos6)){
		
			$image_dir = (empty($centos6) ? '/var/lib/xen/images/ubuntu-virt' : '/boot' );
			
			// Make the download directory
			shell_exec('mkdir -p '.$image_dir.' >> '.$args['log'].' 2>&1');
		
			// Download the kernel from Centos 
			shell_exec('wget -O '.$image_dir.'/vmlinuz-2.6.18-348.12.1.el5xen http://mirror.softaculous.com/virtualizor/xenkernel/vmlinuz-2.6.18-348.12.1.el5xen >> '.$args['log'].' 2>&1');
			
			// Download the image file
			shell_exec('wget -O '.$image_dir.'/initrd-virtualizor.img http://mirror.softaculous.com/virtualizor/xenkernel/initrd-virtualizor.img >> '.$args['log'].' 2>&1');
			@chmod($image_dir.'/initrd-virtualizor.img', 0600);
			
			
		}else{
			// For centos 5 Also
			$image_dir = '/boot';
		}
		
		// Which is the kernel ?
		$files = filelist($image_dir.'/', 0);
		
		foreach($files as $k => $v){
			unset($matches);
			if(preg_match('/^vmlinuz\-(.*?)xen(.*?)/is', $v['name'])){
				$uname = str_replace('vmlinuz-', '', $v['name']);
			}
		}
		
		// Did we find the UNAME ?
		if(!empty($uname)){
		
			echo "	Making vmlinuz-virtualizor and initrd-virtualizor.img \n";
			shell_exec('echo "Making vmlinuz-virtualizor and initrd-virtualizor.img" >> '.$args['log'].' 2>&1');
			
			if(empty($distro) && empty($centos6)){
				// Make the initrd.img
				shell_exec('/sbin/mkinitrd -f --omit-scsi-modules --omit-raid-modules --omit-lvm-modules --with=xennet --with=xenblk --preload=xenblk /boot/initrd-virtualizor.img '.$uname);
			}
			
			// Link the Kernel
			shell_exec('/bin/ln -sf '.$image_dir.'/vmlinuz-'.$uname.' '.$image_dir.'/vmlinuz-virtualizor');
		
		}
		
		if($distro == 'ubuntu'){
			
			// Update the guestfs appliance
			@shell_exec('update-guestfs-appliance >> '.$args['log'].' 2>&1');
			
			exec('lsb_release -r', $out1, $ret1);
			if(preg_match('/14.04/is', $out1[0])){
				$kernel_version = '4.4';
			}else{
				$kernel_version = '4.1';
			}
			
			//Setup GRUB to boot the Xen Hypervisor - UBUNTU
			$file = file('/etc/default/grub');
			foreach($file as $k => $v){
				if(preg_match('/\bGRUB\_DEFAULT\b\=/is', $v)){
					$file[$k] = 'GRUB_DEFAULT="Xen '.$kernel_version.'-'.($arch == 'i686' ? 'i386' : 'amd64').'"'."\n";
				}
				
				if(preg_match('/\bGRUB_CMDLINE_LINUX\b\=/is', $v)){
					$file[$k] = 'GRUB_CMDLINE_LINUX="apparmor=0"'."\n".'GRUB_CMDLINE_XEN="dom0_mem=1G"'."\n";
				}
			}
			
			// Write the changes
			writefile('/etc/default/grub', implode('', $file), 1);
			
			/// Changes made now update the GRUB
			@shell_exec('update-grub >> '.$args['log'].' 2>&1');
			
			// Set the default VM tool - XM
			$file = file('/etc/default/xen');
			
			foreach($file as $k => $v){
				if(preg_match('/\bTOOLSTACK\b\=/is', $v)){
					$file[$k] = 'TOOLSTACK="xm"'."\n";
				}
			}
			
			// Write the changes
			writefile('/etc/default/xen', implode('', $file), 1);
			
			// Make some simlinks
			@shell_exec('/bin/ln -s /usr/lib/xen-'.$kernel_version.' /usr/lib/xen');
			@shell_exec('/bin/ln -s /usr/lib/xen-'.$kernel_version.'/bin/pygrub /usr/bin/pygrub');
			@shell_exec('/bin/ln -s /usr/share/qemu-linaro/ /usr/share/qemu');
			
		}
	}
	
	// Install bridge script for UBUNTU and CENTOS 6 ONLY
	if($distro == 'ubuntu' || !empty($centos6)){
	
		// Install our Bridge Script
		@shell_exec('/bin/ln -s /usr/local/virtualizor/bridge /etc/init.d/virtnetwork >> '.$args['log'].' 2>&1');
		@chmod('/etc/init.d/virtnetwork', 0755);

		// activate the service at startup
		_init_config('virtnetwork', 19);

		// Systemctl Virtnetwork service
		if(file_exists('/usr/bin/systemctl')){
			
			@shell_exec('ln /usr/local/virtualizor/virtnetwork.service /etc/systemd/system/virtnetwork.service >> '.$args['log'].' 2>&1');
			
			@shell_exec('systemctl enable virtnetwork >> '.$args['log'].' 2>&1');
			
		}
	
	}
	
	// IP Forwarding Settings
	sysctl_configure('net.ipv4.ip_forward', 1);
	sysctl_configure('net.ipv6.conf.all.forwarding', 1);
	
	// Ensure the OS Templates folder is there
	@mkosdir();
	
	// Delete virbr0
	shell_exec('/bin/rm -rf /etc/libvirt/qemu/networks/autostart/default.xml >> '.$args['log'].' 2>&1');
	
	$osid = $GLOBALS['latest_os']['xen'];
	
	// Save a OS Template
	$res = makequery("REPLACE INTO `os`
						SET osid = '$osid'");
	
	// Download if allowed
	if(empty($args['noos'])){
		shell_exec('wget -O /var/virtualizor/xen/'.$oses[$osid]['filename'].' '.$oses[$osid]['url'].' >> '.$args['log'].' 2>&1');
	}
	
	return true;
	
}


// Will Install KVM
function install_kvm(){
	
	global $args, $globals, $ostemplates, $oses, $distro;
	
	if(!isset($distro)){	
		$distro	 = get_distro();		
	}
	
	exec('uname -i', $out);
	
	// KVM not allowed in 32 Bit Operating System
	if($out[0] != 'x86_64'){
		echo "	ERROR : KVM can not be installed in 32 Bit Operating System \n";
		shell_exec('echo "ERROR : KVM can not be installed in 32 Bit Operating System" >> '.$args['log'].' 2>&1');
		return false;
	}
	
	// Handle Storage
	if(!install_handle_storage('kvm')){
		return false;	
	}
	
	// Does it support Virtualization
	exec('/bin/cat /proc/cpuinfo | grep vmx', $out, $intel);
	exec('/bin/cat /proc/cpuinfo | grep svm', $out, $amd);
	
	// $args['ovc'] - Override Virtualization Check
	if($intel != '0' && $amd != '0' && empty($args['ovc'])){
		echo "	ERROR : Your CPU doesnt support Hardware Level Virtualization OR you havent enabled Virtualization from the BIOS. Please enable Virtualization from the BIOS and then RE-Run this installer \n";
		shell_exec('echo "ERROR : Your CPU doesnt support Hardware Level Virtualization OR you havent enabled Virtualization from the BIOS. Please enable Virtualization from the BIOS and then RE-Run this installer" >> '.$args['log'].' 2>&1');
		return false;
	}
	
	exec('/sbin/lsmod | grep kvm', $out, $ret);
	
	if($distro == 'ubuntu'){
		exec('lsb_release -r', $out1, $ret1);
	}else{
		$release = file_get_contents('/etc/redhat-release');
	}
	
	// Is it installed or are we in Import Mode
	if($ret != '0' || !empty($args['import']) || ($distro == 'ubuntu' && preg_match('/(14.04|16.04)/is', $out1[0])) || (empty($distro) && preg_match('/release 7/is', $release))){
	
		if($distro == 'ubuntu'){
			
			// For skipping libguestfs-tools interactive mode
			putenv('DEBIAN_FRONTEND=noninteractive');
			
			$list = array("qemu-kvm", "libvirt-bin", "virtinst", "bridge-utils", "ntfs-3g", "sysv-rc-conf", "qemu-utils", "dhcp3-server", "isc-dhcp-server", "libvncserver0", "python-numpy", "chntpw", "qemu-img", "libguestfs-tools", "guestmount", "ebtables", "virt-top");
			
		}elseif(preg_match('/release 7.3/is', $release)){
			
			$list = array("kvm", "kmod-kvm", "qemu-kvm", "ntfsprogs", "ntfs-3g", "libvirt", "virt-top", "bridge-utils", "dhcp", "libvncserver", "numpy", "chntpw", "qemu-img", "libconfig", "perl-Sys-Guestfs-1.28*", "libguestfs-1.28*", "libguestfs-tools-1.28*", "libguestfs-tools-c-1.28*", "ebtables", "libguestfs-winsupport", "net-tools");
			
		}else{
		
			// yum install guestfish = libguestfs-tools-c and changes are related to 7
			// We have removed the guestfish as libguestfs-tools-c install guestfish functionality and libconfig need to be install seperatly for centos 7 for other it is installed as dependancy.
			$list = array("kvm", "kmod-kvm", "qemu-kvm", "ntfsprogs", "ntfs-3g", "libvirt", "virt-top", "bridge-utils", "dhcp", "libvncserver", "numpy", "chntpw", "qemu-img", "libconfig", "libguestfs-tools", "libguestfs-tools-c", "ebtables", "libguestfs-winsupport", "net-tools");
			
		}
		
		// Install the packages
		_package_install($list, $args['log']);
		
		// Enable DHCP
		_init_config(($distro == 'ubuntu' ? 'isc-dhcp-server' : 'dhcpd'), 19);
		
		// If Redhat 7 & service = dhcpd 
		if(file_exists('/usr/bin/systemctl')){
			shell_exec('systemctl enable dhcpd 2> /dev/null');
			shell_exec('systemctl enable libvirt 2> /dev/null');	
		}
		
		echo "	KVM Module has been installed \n";
		shell_exec('echo "KVM Module has been installed" >> '.$args['log'].' 2>&1');
		
	}else{
		echo "	KVM already installed ... Continuing \n";
		shell_exec('echo "KVM already installed ... Continuing" >> '.$args['log'].' 2>&1');
	}
	
	// Was libvirt installed ?
	if(!is_dir('/etc/libvirt')){
		echo "	Errors while installing libvirt for KVM \n";
		return false;
	}
	
	// Install our Bridge Script
	@shell_exec('/bin/ln -s /usr/local/virtualizor/bridge /etc/init.d/virtnetwork >> '.$args['log'].' 2>&1');
	@chmod('/etc/init.d/virtnetwork', 0755);
	
	// activate the service at startup
	_init_config('virtnetwork', 97);
	
	// Systemctl Virtnetwork service
	if(file_exists('/usr/bin/systemctl')){
		
		@shell_exec('ln /usr/local/virtualizor/virtnetwork.service /etc/systemd/system/virtnetwork.service >> '.$args['log'].' 2>&1');
		
		@shell_exec('systemctl enable virtnetwork >> '.$args['log'].' 2>&1');
		
	}
	
	@shell_exec('yum -y update  >> '.$args['log'].' 2>&1');
	
	// If User wants to install Nested Virtualization.
	if(!empty($args['nested_virt']) && (preg_match('/release 6/is', $release) || preg_match('/release 7/is', $release))){
		
		exec('/bin/cat /proc/cpuinfo | grep " ept "', $out, $ept);
		exec('/bin/cat /proc/cpuinfo | grep " npt "', $out, $npt);
		
		// If it is CentOS 6 ONLY then we have to install the kernel
		if(($ept == '0' || $npt == '0')){
			
			if(preg_match('/release 6/is', $release)){
		
				//echo 'wget -N http://dev.centos.org/centos/6/xen-c6/xen-c6.repo -O /etc/yum.repos.d/xen-c6.repo';
				@shell_exec('wget -N http://dev.centos.org/centos/6/xen-c6/xen-c6.repo -O /etc/yum.repos.d/xen-c6.repo >> '.$args['log'].' 2>&1');
				@shell_exec('yum -y --enablerepo xen-c6 install kernel kernel-firmware >> '.$args['log'].' 2>&1');
				
			}
		
			// This file we have to write in both CentOS 6 and 7
			if(!file_exists('/etc/modprobe.d/kvm-nested.conf')){
				$str = 'options kvm_intel nested=1'."\n";
				writefile('/etc/modprobe.d/kvm-nested.conf', $str, 1);
			}
			
		}else{
			echo "\nNOT INSTALLING NESTED VIRTUALIZATION AS YOUR SYSTEM DOES NOT MEET THE REQUIREMENT FOR THE NESTED VIRTUALIZATION.\n";
		}
	}
	
	// IP Forwarding Settings
	sysctl_configure('net.ipv4.ip_forward', 1);
	sysctl_configure('net.ipv6.conf.all.forwarding', 1);
	
	// Ensure the OS Templates folder is there
	@mkosdir();
	
	// Delete virbr0
	shell_exec('/bin/rm -rf /etc/libvirt/qemu/networks/autostart/default.xml >> '.$args['log'].' 2>&1');
	
	$osid = $GLOBALS['latest_os']['kvm'];
	
	// Save a OS Template
	$res = makequery("REPLACE INTO `os`
						SET osid = '$osid'");
	
	// Download if allowed
	if(empty($args['noos'])){
		shell_exec('wget -O /var/virtualizor/kvm/'.$oses[$osid]['filename'].' '.$oses[$osid]['url'].' >> '.$args['log'].' 2>&1');
	}
	
	// Edit the libvirt conf - Add the Sleep 5 after pre start script - Ubuntu
	if(file_exists('/etc/init/libvirt-bin.conf') && !empty($distro)){
	
		$file = file('/etc/init/libvirt-bin.conf');
		foreach($file as $k => $v){
			if(preg_match('/\bpre-start\b/is', $v)){
				$file[$k] = 'pre-start script'."\n\t".'sleep 5'."\n";
			}
		}
		// Write the changes
		writefile('/etc/init/libvirt-bin.conf', implode('', $file), 1);
	}
	
	// Is the distro ubuntu ?
	if($distro == 'ubuntu'){
		
		// Update the guestfs appliance
		@shell_exec('update-guestfs-appliance >> '.$args['log'].' 2>&1');
		
		// Replace the new sfdisk with our sfdisk for ubuntu 16
		if(preg_match('/(16.04)/is', $out1[0])){
			new_sfdisk();
		}
		
	}
	
	return true;

}

function install_xcp(){
	global $args, $globals, $ostemplates, $oses, $distro;
	
	// Check for XCP 
	if(file_exists('/etc/redhat-release')){
	
		exec('cat /etc/redhat-release',$output);
		
		if(!preg_match('/xenserver/i',$output[0])){
			echo "XenServer can be only installed on XenServer OS.\n";
			shell_exec('echo "XenServer can be only installed on XenServer OS." >> '.$args['log'].' 2>&1');
			return false;
		}
		
	}else{
		return false;
	}
	
	// Search the LV Group
	exec($globals['com']['vgdisplay'].' '.$globals['lv'].' >> /dev/null 2>&1', $vgout, $vgret);
	
	if($vgret != 0){
		echo "	ERROR : Logical Volume Group NOT FOUND \n Please specify the correct Logical Volume Group and then RE-Run this installer \n";
		shell_exec('echo "ERROR : Logical Volume Group NOT FOUND \n Please specify the correct Logical Volume Group and then RE-Run this installer" >> '.$args['log'].' 2>&1');
		return false;
	}
	
	// DO YOU have enough space IN IT
	exec($globals['com']['vgdisplay'].' -C --nosuffix -o Free --units K "'.$globals['lv'].'" 2>/dev/null', $volgr);
	
	if(empty($volgr[1])){
		echo "	ERROR : Logical Volume Group - '".$globals['lv']."' is FULL\n Please specify a Logical Volume Group which has enough space to create VPS(s) \n";
		shell_exec('echo "ERROR : Logical Volume Group - "'.$globals['lv'].'" is FULL\n Please specify a Logical Volume Group which has enough space to create VPS(s)" >> '.$args['log'].' 2>&1');
		return false;
	}
	
	if(empty($args['repo_size'])){
		echo "	ERROR : Repository Size Undefined - \n Please specify a Repository size for the media. \n";
		shell_exec('echo "ERROR : Repository Size Undefined - \n Please specify a Repository size for the media. \n" >> '.$args['log'].' 2>&1');
		return false;
	}else{
		
		if(preg_match('/XenServer release 7/i',$output[0])){
			exec("sed -i 's/metadata_read_only = 1/metadata_read_only = 0/g' /etc/lvm/lvm.conf");
		}
		
		// Install a package
		_package_install(array('ntfsprogs', 'chntpw', "qemu-img"), $args['log']);
		
		$virt_media_vg = empty($args['virtmedia_VG']) ? $globals['lv'] : $args['virtmedia_VG'];

		exec($globals['com']['lvcreate'].' -L '.$args['repo_size'].'G -n virtmedia '.$virt_media_vg. ' --config global{metadata_read_only=0}', $repo, $ret_val);
		
		if($ret_val != 0){
			$error_repo = implode('', $repo);
			echo "	ERROR : Repository Size - Could not create Repository due to following error : $ret_val - ".$error_repo;
			shell_exec('echo "ERROR : Repository Size - Could not create Repository due to following error '.$ret_val.' - '.$error_repo.'" >> '.$args['log'].' 2>&1');
			return false;
		}

		// Template and ISO repo creation process
		exec('mkfs.ext3 /dev/'.$virt_media_vg.'/virtmedia 2>/dev/null');
		exec('mkdir -p /var/virtualizor 2>/dev/null');
		exec('/sbin/vgchange -ay --config global{metadata_read_only=0} 2>/dev/null');
		exec('mount /dev/'.$virt_media_vg.'/virtmedia /var/virtualizor 2>/dev/null');
		
		exec('mkdir -p /var/virtualizor/iso 2>/dev/null');
		
		exec('/usr/bin/xe host-list name-label=$(hostname) --minimal', $hostuuid);
		exec('/usr/bin/xe sr-create name-label=ISO-Repo type=iso device-config:location=/var/virtualizor/iso device-config:legacy_mode=true content-type=iso host-uuid='.$hostuuid[0]);
		
		// For Xenserver 6
		if(!preg_match('/XenServer release 7/i',$output[0])){
			// Startup changes for xcp template - Add to FSTAB
			exec('echo "/dev/'.$virt_media_vg.'/virtmedia /var/virtualizor		ext3		defaults	0 0" >> /etc/fstab');
			exec('echo "/usr/sbin/lvm lvchange -ay '.$virt_media_vg.'/virtmedia" >> /etc/rc.local');
			exec('echo "sleep 5" >> /etc/rc.local');
			exec('echo "/bin/mount /dev/'.$virt_media_vg.'/virtmedia" >> /etc/rc.local');
		
		// For Xenserver 7
		}else{
			// Install our Virtmedia mount script
			@shell_exec('/bin/ln -s /usr/local/virtualizor/aaxcpvirtmedia /etc/init.d/aaxcpvirtmedia >> '.$args['log'].' 2>&1');
			@chmod('/etc/init.d/aaxcpvirtmedia', 0755);

			// activate the service at startup
			_init_config('aaxcpvirtmedia', 99);
		}
		
		// Script for enabling client side vnc
		@chmod('/usr/local/virtualizor/scripts/tunnel_shell', 0755);
		
	}
	
	// Get the vhd-util for xcp 
	// Make the xcp directory if not there
	if(!is_dir('/usr/local/virtualizor-bin/xcp')){
		mkdir('/usr/local/virtualizor-bin/xcp', 0755, true);
	}
	
	shell_exec('wget -O /usr/local/virtualizor-bin/xcp/vhd-util.tgz http://mirror.softaculous.com/virtualizor/vhd-util.tgz >> '.$args['log'].' 2>&1');
	
	shell_exec('cd /usr/local/virtualizor-bin/xcp/ && tar -zxvf vhd-util.tgz >> '.$args['log'].' 2>&1');
	
	$osid = $GLOBALS['latest_os']['xcp'];

	// Save a OS Template
	$res = makequery("REPLACE INTO `os`
						SET osid = '$osid'");
	
	
	exec('mkdir -p /var/virtualizor/xcp 2>/dev/null');
	
	// Download if allowed
	if(empty($args['noos'])){
		shell_exec('wget -O /var/virtualizor/xcp/'.$oses[$osid]['filename'].' '.$oses[$osid]['url'].' >> '.$args['log'].' 2>&1');
	}
	
	return true;
}

// Installs LXC
function install_lxc(){

	global $args, $globals, $ostemplates, $oses, $distro;
	if(!isset($distro)){	
		$distro	 = get_distro();		
	}
			
	exec('uname -i', $out);
	
	// LXC not allowed in 32 Bit Operating System
	if($out[0] != 'x86_64'){
		echo "ERROR : Lxc can not be installed in 32 Bit Operating System \n";
		shell_exec('echo "ERROR : Lxc can not be installed in 32 Bit Operating System" >> '.$args['log'].' 2>&1');
		return false;
	}
	
	// Handle Storage
	if(!install_handle_storage('lxc')){
		return false;	
	}
		
	if($distro == 'ubuntu'){
	
		// For skipping libguestfs-tools interactive mode
		putenv('DEBIAN_FRONTEND=noninteractive');
				
		$list = array("lxc", "bridge-utils");
		
	}else{
	
		_package_install('epel-release', $args['log']);
		
		$list = array("lxc", "fuse-devel.x86_64", "pam-devel.x86_64", "pam.x86_64", "bridge-utils", "libvncserver", "ebtables", "net-tools", "libconfig");
	}
	
	// Install the packages
	_package_install($list, $args['log']);
	
	// Was it installed ?
	if(!file_exists('/etc/lxc/default.conf')){
		echo "Errors while installing Lxc \n";
		shell_exec('echo "Errors while installing Lxc" >> '.$args['log'].' 2>&1');
		return false;
	}
	
	// Download LXCFS and untar it
	@shell_exec('mkdir /usr/local/virtualizor-bin >> '.$args['log'].' 2>&1');
	@shell_exec('mkdir -p /usr/local/virtualizor-bin/var/lib/lxcfs/ >> '.$args['log'].' 2>&1');
	@shell_exec('wget http://files.virtualizor.com/lxcfsCompiled.tar.gz -P /usr/local/virtualizor/ >> '.$args['log'].' 2>&1');
	@shell_exec('tar -xvzf /usr/local/virtualizor/lxcfsCompiled.tar.gz -C /usr/local/virtualizor-bin/ >> '.$args['log'].' 2>&1');
	@shell_exec('rm -rf /usr/local/virtualizor/lxcfsCompiled.tar.gz >> '.$args['log'].' 2>&1');
	
	// Mount lxcfs
	@shell_exec('/usr/local/virtualizor-bin/bin/lxcfs /usr/local/virtualizor-bin/var/lib/lxcfs/ >> '.$args['log'].' 2>&1 &');
	
	//Symlink the lxcfs path
	@shell_exec('/bin/ln -s /usr/local/virtualizor-bin/etc/rc.d/init.d/lxcfs /etc/rc.d/init.d/lxcfs >> '.$args['log'].' 2>&1');
	
	// Install our Bridge Script
	@shell_exec('/bin/ln -s /usr/local/virtualizor/bridge /etc/init.d/virtnetwork >> '.$args['log'].' 2>&1');
	@chmod('/etc/init.d/virtnetwork', 0755);
	
	// activate the service at startup
	_init_config('virtnetwork', 97);
	_init_config('lxcfs', 98);
	
	// Systemctl Virtnetwork service
	if(file_exists('/usr/bin/systemctl')){
		
		@shell_exec('ln /usr/local/virtualizor/virtnetwork.service /etc/systemd/system/virtnetwork.service >> '.$args['log'].' 2>&1');
		
		@shell_exec('systemctl enable virtnetwork >> '.$args['log'].' 2>&1');
	}
	
	@shell_exec('yum -y update  >> '.$args['log'].' 2>&1');
	
	// IP Forwarding Settings
	sysctl_configure('net.ipv4.ip_forward', 1);
	sysctl_configure('net.ipv6.conf.all.forwarding', 1);
	
	// Ensure the OS Templates folder is there
	@mkosdir();
	
	$osid = $GLOBALS['latest_os']['lxc'];
	
	// Save a OS Template
	$res = makequery("REPLACE INTO `os`
						SET osid = '".$osid."'");
	
	// Download if allowed
	if(empty($args['noos'])){
		shell_exec('wget -O /var/virtualizor/lxc/'.$oses[$osid]['filename'].' '.$oses[$osid]['url'].' >> '.$args['log'].' 2>&1');
	}
	
	return true;
	
}

function install_virtuozzo(){

	global $args, $globals, $ostemplates, $oses, $distro;
	
	// Ensure the OS Templates folder is there
	@mkosdir();
	
	$ovz_osid = $GLOBALS['latest_os']['vzo'];
	$kvm_osid = $GLOBALS['latest_os']['vzk'];
	
	// Save a Openvz OS Template
	$res = makequery("INSERT INTO `os`
						SET osid = '$ovz_osid'");
	
	// Save a OS Template
	$res = makequery("INSERT INTO `os`
						SET osid = '$kvm_osid'");
	
	$storage = array();
	
	// OpenVZ Storage
	$storage[0]['name'] = 'Default Storage OpenVZ';
	$storage[0]['path'] = '/vz/private';
	$storage[0]['type'] = 'openvz';
	$storage[0]['format'] = '';
	$storage[0]['primary_storage'] = 1;	
	
	// KVM Storage
	$storage[1]['name'] = 'Default Storage KVM';
	$storage[1]['path'] = '/vz/vmprivate';
	$storage[1]['type'] = 'file';
	$storage[1]['format'] = 'qcow2';
	$storage[1]['primary_storage'] = 1;
	
	virtuozzo_add_storage($storage);
	
	// Download if allowed
	if(empty($args['noos'])){
		shell_exec('wget -O /var/virtualizor/vzk/'.$oses[$kvm_osid]['filename'].' '.$oses[$kvm_osid]['url'].' >> '.$args['log'].' 2>&1');
		shell_exec('wget -O /vz/template/cache/'.$oses[$ovz_osid]['filename'].' '.$oses[$ovz_osid]['url'].' >> '.$args['log'].' 2>&1');
	}
	
	if(!is_dir('/vz/vmprivate')){
		@shell_exec('mkdir /vz/vmprivate >> '.$args['log'].' 2>&1');
	}
	
	if(!is_dir('/vz/private')){
		@shell_exec('mkdir /vz/private >> '.$args['log'].' 2>&1');
	}
	
	return true;

}

function virtuozzo_add_storage($st){

	foreach($st as $sk => $sv){
	
		$newstid = insert_and_id("INSERT INTO storage
				SET name = :name,
				`st_uuid` = :st_uuid,
				path = :path,
				`type` = :type,
				format = :format,
				primary_storage = :primary_storage",
				array(':name' => $sv['name'],
					':st_uuid' => generateRandStr(16),
					':path' => $sv['path'],
					':type' => $sv['type'],
					':format' => $sv['format'],
					':primary_storage' => $sv['primary_storage']));
				
		// Add the relationship
		$res = insert_and_id("INSERT INTO storage_servers
					SET stid = '".$newstid."',
					serid = '0',
					sgid = '-2'");
	}	

}


// Installs PROXMOX
function install_proxmox(){
	global $args, $globals, $ostemplates, $oses, $distro;
	
	//Get Proxmox version
	exec('/usr/bin/pveversion -v | grep pve-manager', $proxV);
	
	if(preg_match("/3\.[0-4]+-/is", $proxV[0])){
		$prox_v = 3;
	}else{
		$prox_v = 4;
	}
	
	// Handle Storage
	if(!install_handle_storage('proxmox')){
		return false;	
	}
	
	$list = array("chntpw", "ntfs-3g", "python-numpy", "ebtables", "isc-dhcp-server", "sysv-rc-conf");
	
	_package_install($list, $args['log']);
	
	// Override sfdisk for proxmox 5
	if(preg_match("/5\.[0-4]+-/is", $proxV[0])){
		new_sfdisk();
	}
	
	if($prox_v == 3){
		// Download Libguestfs for proxmox 3.x and untar it
		@shell_exec('mkdir -p /usr/local/virtualizor-bin >> '.$args['log'].' 2>&1');
		@shell_exec('wget http://files.virtualizor.com/proxmox/libguestfsCompiled3.tar.gz -P /usr/local/virtualizor/ >> '.$args['log'].' 2>&1');
		@shell_exec('tar -xvzf /usr/local/virtualizor/libguestfsCompiled3.tar.gz -C /usr/local/virtualizor-bin >> '.$args['log'].' 2>&1');
		@shell_exec('rm -rf /usr/local/virtualizor/libguestfsCompiled3.tar.gz >> '.$args['log'].' 2>&1');
	}else{
		// Download Libguestfs for proxmox 4.x or proxmox 5.x and untar it
		@shell_exec('mkdir -p /usr/local/virtualizor-bin >> '.$args['log'].' 2>&1');
		@shell_exec('wget http://files.virtualizor.com/proxmox/libguestfsCompiled4.tar.gz -P /usr/local/virtualizor/ >> '.$args['log'].' 2>&1');
		@shell_exec('tar -xvzf /usr/local/virtualizor/libguestfsCompiled4.tar.gz  -C /usr/local/virtualizor-bin >> '.$args['log'].' 2>&1');
		@shell_exec('rm -rf /usr/local/virtualizor/libguestfsCompiled4.tar.gz  >> '.$args['log'].' 2>&1');
	}
	
	// Start cron service 
	@shell_exec('systemctl start cron >> '.$args['log'].' 2>&1');
	
	// Enable DHCP
	_init_config('isc-dhcp-server' , 19);
	
	// IP Forwarding Settings
	sysctl_configure('net.ipv4.ip_forward', 1);
	sysctl_configure('net.ipv6.conf.all.forwarding', 1);
	
	// Symlink os and iso directory before directory creation
	symlink('/var/lib/vz/template/iso', $globals['isos']);
	
	// Create symlink for /vz
	symlink('/var/lib/vz', '/vz');
	
	if($prox_v == 3){
		symlink('/var/lib/vz/template/cache/', $globals['proxoos']);	
	}else{
		symlink('/var/lib/vz/template/cache/', $globals['proxlos']);
	}
	
	// Ensure the OS Templates folder is there
	@mkosdir();
	
	return true;
	
}

// New sfdisk for ubuntu 16.04 and proxmox 5.0(debain stretch)
function new_sfdisk(){
	global $args;
	
	// Rename the new sfdisk
	shell_exec('mv /sbin/sfdisk /sbin/sfdisk_new >> '.$args['log'].' 2>&1');
	
	// Download and save our sfdisk
	shell_exec('wget -O /sbin/sfdisk http://files.virtualizor.com/utility/sfdisk >> '.$args['log'].' 2>&1');
	
	@chmod('/sbin/sfdisk', 0755);
}

// Verifies the email address
function _emailvalidation($email){
	if(!preg_match("/^([a-zA-Z0-9])+([a-zA-Z0-9\+._-])*@([a-zA-Z0-9_-])+([.])+([a-zA-Z0-9\._-]+)+$/", $email)){	
		return false;	
	}else{	
		return true;	
	}
}

// Installer function. works for RHEL and Debian based systems
// Params : $package : array or string provided with packages name
// $distro : Centos or Ubuntu. 
// $log : log path 

function _package_install($package, $log){
	
	global $globals, $distro;
	
	$package = (is_array($package) ? $package : array($package));
	
	$package_list = implode(' ', $package);
	
	if($distro == 'ubuntu' || $distro == 'proxmox'){
		foreach($package as $k => $v){
			shell_exec('apt-get -y install '.$v.' >> '.$log.' 2>&1');
		}
	}else{
		shell_exec('yum -y install '.$package_list.' >> '.$log.' 2>&1');
	}

}


// chkconfig script.
// If priority given, will be applied on UBUNTU only.
function _init_config($service, $priority = ''){

	global $globals, $distro;
	
	// run according to distro
	if($distro == 'ubuntu' || $distro == 'proxmox'){
		shell_exec('update-rc.d '.$service.' defaults '.$priority.'');
		
	// for centos based
	}else{
		shell_exec('/sbin/chkconfig '.$service.' on');
	}
}

// Install Comman packages

function install_common_packages(){
	
	global $args, $globals, $ostemplates, $oses, $distro;
	
	$arch = exec('arch');
	
	$list = array(); // Common Packages needed to be installed
	
	if($distro == 'ubuntu' || $distro == 'proxmox'){
		
		$throttlearch = 'amd64';
		if($arch == 'i686'){
			$throttlearch = 'i386';
		}
		
		shell_exec('wget http://mirror.softaculous.com/virtualizor/debian/pool/main/throttle_1.2-5_'.$throttlearch.'.deb >> '.$args['log'].' 2>&1');
		
		shell_exec('dpkg -i throttle_1.2-5_'.$throttlearch.'.deb >> '.$args['log'].' 2>&1');
	}else{
		$list[] = 'throttle';
	}
	
	// Install the packages
	_package_install($list, $args['log']);
	
}

// Get the current distro
function get_distro(){

	// find the destro ubuntu or centos
	if(file_exists('/etc/redhat-release')){
		// Its CentOS
		$distro = '';
	}elseif(file_exists('/etc/debian_version')){
		// Its Ububtu
		$distro = 'ubuntu';
	}
	
	return $distro;

}

// Are we in MULTI Virt mode ?
if(defined('VIRTUALIZOR_MULTI')){
	return true;
}

//****************************
// Starting the INSTALL CODE
//****************************

if(empty($argv)){
	die('');
}

// The Log File
$args['log'] = '/root/virtualizor.log';

foreach($argv as $k => $v){
	$v = explode('=', $v);
	if(empty($v[1])) continue;
	$args[$v[0]] = $v[1];
}

// If no kernel specified then it is OpenVZ
if(empty($args['kernel'])){
	$args['kernel'] = 'openvz';
}

// Is it multi kernel
$args['kernel'] = str_replace('-', ' ', $args['kernel']);

// for virtuozzo now we will use openvz7
if($args['kernel'] == 'openvz7'){
	$args['kernel'] = 'virtuozzo';
}

// Check for allowed kernel arguments
$allowed_virts = array('kvm lxc', 'kvm openvz', 'openvz', 'kvm', 'lxc', 'xen', 'xcp', 'virtuozzo', 'proxmox');

if(!in_array($args['kernel'], $allowed_virts) && empty($args['master'])){
	echo "ERROR : Invalid kernel Specified : ".str_replace(' ', '-', $args['kernel']) ."\n";
	shell_exec('echo "ERROR : Invalid kernel Specified : '.str_replace(' ', '-', $args['kernel']).'"  >> '.$args['log'].' 2>&1');
	exit(16);
}

// Some OS specific commands :
$globals['com']['vgdisplay'] = (is_file('/sbin/vgdisplay') ? '/sbin/vgdisplay' : '/usr/sbin/vgdisplay');

// Is it SolusVM ?
if(file_exists('/usr/local/solusvm')){
	
	echo "		- SolusVM Detected\n";
	
	$args['import'] = 'solusvm';

	// Is there a bridge - NON OpenVZ ?
	if(!preg_match('/openvz/is', $args['kernel'])){
		$args['bridge'] = (empty($args['bridge']) ? 'br0' : $args['bridge']);
	}
	
}


// Verify the email address
if(!_emailvalidation($args['email'])){
	echo "	ERROR : Email address is not valid. Please give a VALID email address \n";
	shell_exec('echo "ERROR : Email address is not valid. Please give a VALID email address" >> '.$args['log'].' 2>&1');
	exit(16);
}

// find the destro ubuntu or centos
if(file_exists('/etc/redhat-release')){
	// Its CentOS
	$distro = $data['distro'] = '';
	
	$release = file_get_contents('/etc/redhat-release');
	
}elseif(is_dir('/etc/pve')){
	// Its proxmox
	$distro = $data['distro'] = 'proxmox';
	
	//Get Proxmox version
	exec('/usr/bin/pveversion', $proxmox_version);
	if(preg_match("/3.4/is", $proxmox_version[0])){
		$prox_v = 3;
	}else{
		$prox_v = 4;
	}
}elseif(file_exists('/etc/debian_version')){
	// Its Ububtu
	$distro = $data['distro'] = 'ubuntu';
}

// Is virtuozzo installed ?
if(!preg_match('/Virtuozzo/is', $release) && (preg_match('/virtuozzo/is', $args['kernel']))){
	echo "Virtuozzo can be installed only in Virtuozzo OS\n";
	shell_exec('echo "Virtuozzo can be installed only in Virtuozzo OS" >> '.$args['log'].' 2>&1');
	return false;
}

// LXC check for CentOS 6
if(preg_match('/release 6/is', $release) && (preg_match('/lxc/is', $args['kernel'])) && empty($args['master'])){
	echo "LXC cannot be installed on CentOS 6\n";
	shell_exec('echo "'.ucfirst($args['kernel']).' cannot be installed on CentOS 6" >> '.$args['log'].' 2>&1');
	return false;
}
			
if(preg_match('/release 7/is', $release) && (!preg_match('/kvm/is', $args['kernel'])) && empty($args['master']) && (!preg_match('/(virtuozzo|lxc|xcp|xen)/is', $args['kernel'])) && (!preg_match('/proxmox/is', $args['kernel']))){
	echo ucfirst($args['kernel'])." cannot be installed on CentOS 7\n";
	shell_exec('echo "'.ucfirst($args['kernel']).' cannot be installed on CentOS 7" >> '.$args['log'].' 2>&1');
	return false;
}

// Download the Package
shell_exec('cd /root; wget -O 3.0.0.zip "https://s2.softaculous.com/a/virtualizor/updates.php?give=3.0.0.6" >> '.$args['log'].' 2>&1');

// Unzip the Package
shell_exec('cd /root; unzip -o 3.0.0.zip -d /usr/local/virtualizor >> '.$args['log'].' 2>&1');

// Remove the Package
shell_exec('cd /root; /bin/rm -rf 3.0.0.zip >> '.$args['log'].' 2>&1');

// Now copy the CONF files
exec('ln -s /usr/local/virtualizor/conf/emps/nginx.conf /usr/local/emps/etc/nginx/nginx.conf >> '.$args['log'].' 2>&1');
exec('ln -s /usr/local/virtualizor/conf/emps/php-fpm.conf /usr/local/emps/etc/php-fpm.conf >> '.$args['log'].' 2>&1');
exec('ln -s /usr/local/virtualizor/conf/emps/php.ini /usr/local/emps/etc/php.ini >> '.$args['log'].' 2>&1');
exec('ln -s /usr/local/virtualizor/conf/emps/my.cnf /usr/local/emps/etc/my.cnf >> '.$args['log'].' 2>&1');
exec('ln -s /usr/local/virtualizor/conf/emps/emps /etc/init.d/virtualizor >> '.$args['log'].' 2>&1');

// Put the Auto start
@chmod('/usr/local/virtualizor/conf/emps/emps', 0755);
@chmod('/etc/init.d/virtualizor', 0755);

if($distro == 'ubuntu' && $args['kernel'] == 'xen'){
	_init_config('virtualizor', 17);
}else{
	_init_config('virtualizor');
}

// Start MySQL
exec('/usr/local/emps/bin/mysqlctl start >> '.$args['log'].' 2>&1');

// Default is OpenVZ
if(empty($args['kernel'])){	
	$args['kernel'] = 'openvz';
}

@define('VIRTUALIZOR', 1);

//Set error reporting
error_reporting(E_ALL & ~E_DEPRECATED);

///////////////////////////
// TASKS TO DO
// 1) Make universal.php
// 2) Get license.php
// 3) Set the CRON
// 4) Import Database
///////////////////////////

// Include the RAW universal.php
include_once('universal.php');

// Dont Connect!
$globals['dbconnect'] = 1;
//Some global vars
include_once($globals['path'].'/globals.php');

//The necessary functions to run this SOFTACULOUS Software
include_once($globals['mainfiles'].'/functions.php');

// EMPS Updater cron
$first_day = rand(1,10);
$second_day = $first_day + 15;

//===========================
// 1) Make the universal.php
//===========================
echo "		- Configuring Virtualizor\n";

$data['cookie_name'] = 'SIMCookies'.rand(1, 9999);
$data['dbpass'] = generateRandStr(10);
$data['soft_email'] = $args['email'];
$data['kernel'] = ($args['kernel'] == 'virtuozzo' ? 'vzk vzo' : $args['kernel']);
$data['kernel'] = ($data['kernel'] == 'proxmox' ? ($prox_v == 3 ? 'proxk proxo' : 'proxk proxl') : $data['kernel']);
$data['key'] = generateRandStr(32);
$data['pass'] = generateRandStr(32);
$data['lv'] = (empty($args['lvg']) ? '' : $args['lvg']);
//$data['disk_path'] = (empty($args['filedisk']) ? '' : $args['filedisk']);
$data['cron_time'] = rand(1, 59).' '.rand(1, 23).' * * *';
$globals['emps_cron_time'] = rand(1, 59).' '.rand(1, 23).' '.$first_day.','.$second_day.' * *';
$data['novnc'] = 1;
$data['turnoff_virtdf'] = 1;

// Proxmox Bridge
if($args['kernel'] == 'proxmox'){
	$args['bridge'] = (empty($args['bridge']) ? 'vmbr0' : $args['bridge']);
}

// For xen we have to make some chnages
if(preg_match('/xen/is', $args['kernel'])){
	$globals['use_xl'] = 'xl';
	$data['use_xl'] = $globals['use_xl'];
	$globals['xen_dom0mem'] = '1024';
	$data['xen_dom0mem'] = $globals['xen_dom0mem'];
	$globals['xen_pvbridge'] = (empty($args['bridge']) ? 'viifbr0' : $args['bridge']);
	$globals['xen_hvmbridge'] = $globals['xen_pvbridge'];
	$data['xen_pvbridge'] = $globals['xen_pvbridge'];
	$data['xen_hvmbridge'] = $globals['xen_pvbridge'];
}

// Do we have a bridge ?
if(!empty($args['bridge'])){
	$globals['bridge'] = $args['bridge'];
	$data['bridge'] = $globals['bridge'];
}

// Do we have an interface ?
if(!empty($args['interface'])){
	$globals['interface'] = $args['interface'];
	$data['interface'] = $globals['interface'];
}

// Save it and reload as well
saveglobals($data, 1);


//===========================
// 2) Load the License
//===========================
echo "		- Fetching License\n";
loadlicense(1);

if(!file_exists($globals['path'].'/license.php')){
	echo "	ERROR : Could not download the license file \n";
	shell_exec('echo "ERROR : Could not download the license file" >> '.$args['log'].' 2>&1');
	return false;
}

//===========================
// 3) Set the CRON
//===========================
echo "		- Setting up the CRON Job\n";
add_cron($globals['cron_time']);


//===========================
// 4) Import Database
//===========================
echo "		- Importing Database\n";

$globals['conn'] = @mysql_connect($globals['dbhost'], $globals['dbuser'], '', true);
@mysql_select_db('mysql', $globals['conn']) or die( "Unable to select database 1");

// Update the ROOT Password
makequery("UPDATE `user` 
SET `password` = PASSWORD('".$data['dbpass']."')
WHERE `User` = 'root'");

// Drop the Database if its there!
makequery("DROP DATABASE IF EXISTS `".$globals['db']."`");

// Create the Database
makequery("CREATE DATABASE `".$globals['db']."`");

shell_exec('/usr/local/emps/bin/mysqlctl restart');

// Import the Database
shell_exec('/usr/local/emps/bin/mysql -h localhost -u root -p'.$globals['dbpass'].' '.$globals['db'].' < '.$globals['path'].'/virtualizor.sql');

// Reconnect to test
$globals['conn'] = @mysql_connect($globals['dbhost'], $globals['dbuser'], $globals['dbpass'], true);
@mysql_select_db($globals['db'], $globals['conn']) or die( "Unable to select database 2");

// Insert the DB Version
makequery("REPLACE INTO `registry`
SET `name` = 'version',
`value` = '".$globals['version']."'");

//===========================
// 5) OS Templates List
//===========================
echo "		- Getting List of OS templates\n";
oslist(1);


//===========================
// 6) SSL Certificates
//===========================
echo "		- Generating the SSL Certificates\n";
$hostname = shell_exec('hostname');
$hostname = trim($hostname);
shell_exec('/usr/local/emps/bin/openssl genrsa -out /usr/local/virtualizor/conf/virtualizor.key 1024 >> '.$args['log'].' 2>&1');

shell_exec('/usr/local/emps/bin/openssl req -subj /C=US/ST=Berkshire/L=Newbury/O=\'My Company\'/CN=\''.$hostname.'\'/emailAddress='.$args['email'].' -new -key /usr/local/virtualizor/conf/virtualizor.key -out /usr/local/virtualizor/conf/virtualizor.csr >> '.$args['log'].'');

shell_exec('/usr/local/emps/bin/openssl x509 -req -days 365 -in /usr/local/virtualizor/conf/virtualizor.csr -signkey /usr/local/virtualizor/conf/virtualizor.key -out /usr/local/virtualizor/conf/virtualizor.crt >> '.$args['log'].' 2>&1');

// Make a Fake file for the moment
shell_exec('echo "" >> /usr/local/virtualizor/conf/virtualizor-bundle.crt');

//===========================
// 7) Disabling SELinux
//===========================
if(is_dir('/etc/selinux')){
	shell_exec('/bin/mv /etc/selinux/config /etc/selinux/config_ >> '.$args['log'].' 2>&1');
	shell_exec('echo SELINUX=disabled > /etc/selinux/config');
	shell_exec('echo SELINUXTYPE=disabled >> /etc/selinux/config');
}

//===========================
// 8) Installing the KERNEL
//===========================
echo "4) Installing the Virtualization Kernel - ".(!empty($args['master']) ? 'Master' : $args['kernel'])."\n";
shell_exec('echo "4) Installing the Virtualization Kernel - '.(!empty($args['master']) ? 'Master' : $args['kernel']).'" >> '.$args['log'].' 2>&1');

// We are installing Master
if(!empty($args['master'])){
	
	// Save you are master only server
	$data['is_master_only'] = 1;

	// Save it and reload as well
	saveglobals($data, 1);
	
	// Install the master
	if(!install_master()){
		exit(16);
	}
	
}else{
	
	$virts = explode(' ', $args['kernel']);
	
	foreach($virts as $_virt){
	
		// We are installing OpenVZ
		if($_virt == 'openvz'){
			if(!install_openvz()){
				exit(16);
			}
		}
		
		// We are installing Xen Paravirtualization
		if($_virt == 'xen'){
			if(!install_xen()){
				exit(16);
			}
		}
		
		// We are installing Linux KVM
		if($_virt == 'kvm'){
			if(!install_kvm()){
				exit(16);
			}
		}
		
		// We are installing XCP
		if($_virt == 'xcp'){
			if(!install_xcp()){
				exit(16);
			}
		}
		
		// We are installing LXC
		if($_virt == 'lxc'){
			if(!install_lxc()){
				exit(16);
			}
		}
		
		// We are installing virtuozzo
		if($_virt == 'virtuozzo'){
			if(!install_virtuozzo()){
				exit(16);
			}
		}
		
		// We are installing PROXMOX
		if($_virt == 'proxmox'){
			if(!install_proxmox()){
				exit(16);
			}
		}
	}
	
	// Install Comman Packages Needed for all Virt
	install_common_packages();
	
}


// Install our Traffic-control script
@shell_exec('/bin/ln -s /usr/local/virtualizor/zzvirtservice /etc/init.d/zzvirtservice >> '.$args['log'].' 2>&1');
@chmod('/etc/init.d/zzvirtservice', 0755);

// activate the service at startup
_init_config('zzvirtservice', 99);

// Delete the IP range if there
system("rm -f /etc/sysconfig/network-scripts/ifcfg-*-range*");



// Exit with a status of 8 - It was successful
exit(8);

?>
