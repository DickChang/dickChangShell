# ~/.bashrc: executed by bash(1) for non-login shells.

##########################
# GIT Repository variables
##########################
# Repository Storage Location
export GIT_REPOS_ROOT="/home/dickChang/git_repo/"
# GIT Web HTML interface directory
export GIT_WEB_DIR="/home/dickChang/git.dickchang.com/"
# GIT Web install directory
export GITWEB_INSTALL_DIR="/home/dickChang/gitweb/"

####
# Add tmux path to PATH
####
export PATH=$PATH:"~/local/bin"
export LD_LIBRARY_PATH="~/local/lib"

#####
# Prompt Format
# For Date/Time formatting see strftime. \D{format}
#####

parse_git_branch() {
  git branch 2> /dev/null | sed -e '/^[^*]/d' -e 's/* \(.*\)/\1 /'
}

GREY='\[\033[01;30m\]'
NOCOLOR='\[\033[0m\]'
LIGHTGREY='\[\033[00;37m\]'
WHITE='\[\033[01;37m\]'

# Format:
#   debian_chroot
#   pwd
#   [HH:MM:SS AM/PM] $
#PS1="${debian_chroot:+($debian_chroot)}
#${GREY}\w${NOCOLOR}
#[\D{%I:%M:%S %p}] $ "

# Format:
#   debian_chroot
#   pwd
#   [HH:MM:SS AM/PM] git_branch $
#PS1="${debian_chroot:+($debian_chroot)}
#${GREY}\w${NOCOLOR}
#[\D{%I:%M:%S %p}]${GREY} \$(parse_git_branch)${NOCOLOR}$ "

# Format:
#   debian_chroot
#   pwd
#   git_branch [HH:MM:SS AM/PM] $
PS1="${debian_chroot:+($debian_chroot)}
${GREY}\w
\$(parse_git_branch)${NOCOLOR}[\D{%I:%M:%S %p}] $ "
##########
# /Prompt Format
##########


