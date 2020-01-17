complete -o dirnames -C goto g
function g {
    new_path="$(goto -q ${@})"
    if [ -d "${new_path}" ]; then
        echo -e "\\033[32mGoto\\033[0m \\033[34m${new_path}\\033[0m"
        cd "${new_path}"
	goto -a `pwd`
    else
	echo -e "\\033[31mGoto \"${new_path}\" not found"
	goto
	echo -e -n "\\033[0m"
        false
    fi
}

function gl {
    # list all the history
    echo -e "\\033[32mGotoHistory:\\033[0m"
    goto -l
}

function ga {
    # add current dir to history
    goto -a `pwd`
    echo -e "\\033[32mAdd \\033[34m`pwd`\\033[32m to goto history ok!\\033[0m"
}

function gd {
    # del current dir from the history
    goto -d `pwd`
    echo -e "\\033[32mDel \\033[34m`pwd`\\033[32m from goto history ok!\\033[0m"
}

function gc {
    # clear the goto history
    goto -c
    echo -e "\\033[32mClear the goto history ok!\\033[0m"
}
