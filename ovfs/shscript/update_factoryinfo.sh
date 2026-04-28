#!/bin/sh
# Update factoryinfo.json from /root/version.

VerName=/root/version
FileName=/usr/etc/factoryinfo.json;

update_factoryinfo()
{
	if [ ! -f /usr/etc/factoryinfo.json ] ; then
		cp -af /root/res/factoryinfo.json /usr/etc/factoryinfo.json;
	fi
	
	if [ ! -f /usr/etc/factoryinfo.json ] ; then
		FileName=/root/res/factoryinfo.json;
	fi
	
	if [ -f ${VerName} ] ; then
		for subkey in $(cut -d '=' -f 1 ${VerName}) ; do 
			subval=`grep -w "${subkey}" "${VerName}" | cut -d '=' -f 2`;
			echo ${subval} | grep "\""; ret=$?;
			if [ $ret -eq 0 ] ; then
				sed -i -e "s/\"${subkey}\":.*\"/\"${subkey}\":${subval}/g" ${FileName};
			else 
				sed -i -e "s/\"${subkey}\": *[0-9]\{1,\}/\"${subkey}\":${subval}/g" ${FileName};
			fi

		done
		
		rm -rf /update/version root/version;
		
		if [ -f /usr/etc/factoryinfo.json ] ; then
			cp -af ${FileName} /root/res/factoryinfo.json;
			cp -af ${FileName} /update/res/factoryinfo.json;
		else
			cp -af ${FileName} /update/res/factoryinfo.json;
		fi
	fi
}

update_factoryinfo
