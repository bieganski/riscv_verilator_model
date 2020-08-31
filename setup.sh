#!/bin/bash

# set -e

check_not_sudo() {
if [ "$EUID" -eq 0 ]
  then echo "Dont run as root please."
  exit
fi
}

if [ -f "/etc/debian_version" ]; then
        PKG_INSTALL_CMD="apt-get install"
else
	echo "Unsupported system detected, set architecture and PKG_INSTALL_CMD manually!"
        exit
fi

install_riscv_toolchain() {
	echo "### riscv toolchain installing started..."
	sudo $PKG_INSTALL_CMD npm
	sudo npm install --global xpm@latest
	xpm install --global @xpack-dev-tools/riscv-none-embed-gcc@latest
	echo "### riscv toolchain installed successfully!"
}


# it's /home/mateusz/opt/xPacks/@xpack-dev-tools/riscv-none-embed-gcc/8.3.0-1.2.1/.content/bin
# on my PC
find_riscv_path() {
	pushd . > /dev/null
	cd ~/opt/xPacks
	RISCV_PATH=`find $(pwd) -type d | grep content/bin`
	if [ ! -d "$RISCV_PATH" ]
	then
		echo "### Error: could not find RiscV toolchain in ~/opt/path, check xpm bin directory"
		popd > /dev/null
		exit 1
	fi
	echo "### RISCV toolchain installed found in $RISCV_PATH"
	popd > /dev/null
}



add_to_path() {
	echo "### Adding riscv toolchain location to \$PATH variable..."
	if [ ! -f "$HOME/.bashrc" ]; then
		echo "### Error: cannot find .bashrc file!"
		exit 1
	fi

	if ! grep pathmunge ~/.bashrc; then
		cat <<EOF >>~/.bashrc

# https://unix.stackexchange.com/a/217629
# dont edit this function name!!!
pathmunge() {
        if ! echo "\$PATH" | /bin/grep -Eq "(^|:)\$1($|:)" ; then
           if [ "\$2" = "after" ] ; then
              export PATH="\$PATH:\$1"
           else
              export PATH="\$1:\$PATH"
           fi
        fi
}

pathmunge "$RISCV_PATH"
EOF
	fi
echo "### PATH variable updated! type 'source ~/.bashrc' to make it visible!"
}

update_submodules() {
	echo "### Update git submodules..."
	git submodule update --init --recursive
	echo "### Git submodules updated successfully!"
}


end_msg() {
	echo "### setup.sh finished correctly, now perform next steps from SETUP.md,"
	echo "    depending on architecture you will be using (qemu or FPGA)."
}

check_not_sudo
install_riscv_toolchain
find_riscv_path
add_to_path
update_submodules
end_msg
