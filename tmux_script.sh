SESSION=test

tmux new-session -d -s $SESSION 
tmux select-window -t $SESSION:0
tmux rename-window "tmux_launch_script"

tmux split-window -h
tmux split-window -v
tmux send-keys "echo 'insert commands here'" C-m
tmux select-pane -t 0
tmux split-window -v
tmux select-pane -t 0

tmux a -t $SESSION

echo "DONE"
