export PATH="/opt/homebrew/bin:$PATH"

# >>> conda initialize >>>
# !! Contents within this block are managed by 'conda init' !!
__conda_setup="$('/Users/ruth/opt/anaconda3/bin/conda' 'shell.zsh' 'hook' 2> /dev/null)"
if [ $? -eq 0 ]; then
    eval "$__conda_setup"
else
    if [ -f "/Users/ruth/opt/anaconda3/etc/profile.d/conda.sh" ]; then
        . "/Users/ruth/opt/anaconda3/etc/profile.d/conda.sh"
    else
        export PATH="/Users/ruth/opt/anaconda3/bin:$PATH"
    fi
fi
unset __conda_setup
# <<< conda initialize <<<


export NVM_DIR="$HOME/.nvm"
[ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"  # This loads nvm
[ -s "$NVM_DIR/bash_completion" ] && \. "$NVM_DIR/bash_completion"  # This loads nvm bash_completion

# Created by `pipx` on 2024-11-26 14:53:21
export PATH="$PATH:/Users/ruth/.local/bin"

export OpenMP_ROOT=$(brew --prefix)/opt/libomp

