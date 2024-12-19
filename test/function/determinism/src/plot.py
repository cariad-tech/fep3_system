# Copyright 2023 CARIAD SE. 
# 
# This Source Code Form is subject to the terms of the Mozilla
# Public License, v. 2.0. If a copy of the MPL was not distributed
# with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import csv

###################################
# Getting the data from the files #
###################################

def read_sender_file(file):
    X_real = []
    X_sim = []
    Y = []
    with open(file) as csvfile:
        reader = csv.reader(csvfile, delimiter=' ')
        for row in reader:
            X_real.append(int(row[0]))
            X_sim.append(int(row[1]))
            Y.append(int(row[2]))
    return X_real, X_sim, Y

def read_receiver_file(file):
    X_real = []
    X_sim = []
    Y_signal_one = []
    Y_signal_two = []
    Y_total = []
    with open(file) as csvfile:
        reader = csv.reader(csvfile, delimiter=' ')
        for row in reader:
            X_real.append(int(row[0]))
            X_sim.append(int(row[1]))
            Y_signal_one.append(int(row[2]))
            Y_signal_two.append(int(row[3]))
            Y_total.append(int(row[4]))
    return X_real, X_sim, Y_signal_one, Y_signal_two, Y_total

sender_one_X_real, sender_one_X_sim, sender_signal_one = read_sender_file('sender_one_no_delay.txt')
sender_two_X_real, sender_two_X_sim, sender_signal_two = read_sender_file('sender_two_no_delay.txt')
receiver_X_real, receiver_X_sim, receiver_signal_one, receiver_signal_two, receiver_total = read_receiver_file('receiver_no_delay.txt')
expected_X_real, expected_X_sim, expected_signal_one, expected_signal_two, expected_total = read_receiver_file('expected_values.txt')

#delay_sender_one_X_real, delay_sender_one_X_sim, delay_sender_signal_one = read_sender_file('delayed_sender_one.txt')
#delay_sender_two_X_real, delay_sender_two_X_sim, delay_sender_signal_two = read_sender_file('delayed_sender_two.txt')
#delay_receiver_X_real, delay_receiver_X_sim, delay_receiver_signal_one, delay_receiver_signal_two, delay_receiver_total = read_receiver_file('delayed_receiver.txt')

####################################
# Create Plots for Simulation Time #
####################################

fig, axs = plt.subplots(3,1, sharex='all', sharey='row')
gs = axs[2].get_gridspec()

axs[0].set_title('No delay')
axs[0].plot(expected_X_sim, expected_signal_one, '.-', label='Expected', color='xkcd:light grey')
axs[0].plot(sender_one_X_sim, sender_signal_one, '.-', label='Sender', color='xkcd:light blue')
axs[0].plot(receiver_X_sim, receiver_signal_one, '.-', label='Receiver', color='xkcd:cerulean')
axs[0].set_ylabel('Signal 1')
axs[0].tick_params(labelbottom=True)
axs[0].xaxis.set_major_locator(ticker.MaxNLocator(integer=True))
axs[0].xaxis.set_minor_locator(ticker.MultipleLocator(1))
axs[0].legend()
axs[0].grid(True)

axs[1].plot(expected_X_sim, expected_signal_two, '.-', label='Expected', color='xkcd:light grey')
axs[1].plot(sender_two_X_sim, sender_signal_two, '.-', label='Sender', color='xkcd:light blue')
axs[1].plot(receiver_X_sim, receiver_signal_two, '.-', label='Receiver', color='xkcd:cerulean')
axs[1].set_ylabel('Signal 2')
axs[1].set_xlabel('Simulation time (ms)')
axs[1].tick_params(labelbottom=True)
axs[1].legend()
axs[1].grid(True)

#axs[0][1].set_title('Delayed')
#axs[0][1].plot(delay_sender_one_X_sim, delay_sender_signal_one, '.-', label='Sender', color='xkcd:light orange')
#axs[0][1].plot(delay_receiver_X_sim, delay_receiver_signal_one, '.-', label='Receiver', color='xkcd:orange')
#axs[0][1].tick_params(labelbottom=True, labelleft=True)
#axs[0][1].legend()
#axs[0][1].grid(True)

#axs[1][1].plot(delay_sender_two_X_sim, delay_sender_signal_two, '.-', label='Sender', color='xkcd:light orange')
#axs[1][1].plot(delay_receiver_X_sim, delay_receiver_signal_two, '.-', label='Receiver', color='xkcd:orange')
#axs[1][1].set_xlabel('Simulation time (ms)')
#axs[1][1].tick_params(labelbottom=True, labelleft=True)
#axs[1][1].legend()
#axs[1][1].grid(True)

axs[2].remove()
#axs[2][1].remove()

ax = fig.add_subplot(gs[2,:], sharex = axs[0])
ax.plot(expected_X_sim, expected_total, '.-', label='Expected Total', color='xkcd:light grey')
ax.plot(receiver_X_sim, receiver_total, '.-', label='Total = Signal 1 - Signal 2', color='xkcd:cerulean')
#ax.plot(delay_receiver_X_sim, delay_receiver_total, '.-', label='Total delayed', color='xkcd:orange')
ax.set_xlabel('Simulation time (ms)')
ax.legend()
ax.grid(True, which='both')
ax.grid(which='minor', lw=0.4, ls='--')

##############################
# Create Plots for Real Time #
##############################

fig2, axs2 = plt.subplots(3,1, sharex='all', sharey='row')
gs2 = axs2[2].get_gridspec()

axs2[0].set_title('No Delay')
axs2[0].plot(expected_X_real, expected_signal_one, '.-', label='Expected', color='xkcd:light grey')
axs2[0].plot(sender_one_X_real, sender_signal_one, '.-', label='Sender', color='xkcd:light blue')
axs2[0].plot(receiver_X_real, receiver_signal_one, '.-', label='Receiver', color='xkcd:cerulean')
axs2[0].set_ylabel('Signal 1')
axs2[0].tick_params(labelbottom=True)
axs2[0].xaxis.set_major_locator(ticker.MaxNLocator(integer=True))
axs2[0].legend()
axs2[0].grid(True)

axs2[1].plot(expected_X_real, expected_signal_two, '.-', label='Expected', color='xkcd:light grey')
axs2[1].plot(sender_two_X_real, sender_signal_two, '.-', label='Sender', color='xkcd:light blue')
axs2[1].plot(receiver_X_real, receiver_signal_two, '.-', label='Receiver', color='xkcd:cerulean')
axs2[1].set_ylabel('Signal 2')
axs2[1].set_xlabel('Real time (ms)')
axs2[1].tick_params(labelbottom=True)
axs2[1].legend()
axs2[1].grid(True)

#axs2[0][1].set_title('Delayed')
#axs2[0][1].plot(delay_sender_one_X_real, delay_sender_signal_one, '.-', label='Sender', color='xkcd:light orange')
#axs2[0][1].plot(delay_receiver_X_real, delay_receiver_signal_one, '.-', label='Receiver', color='xkcd:orange')
#axs2[0][1].tick_params(labelbottom=True, labelleft=True)
#axs2[0][1].legend()
#axs2[0][1].grid(True)

#axs2[1][1].plot(delay_sender_two_X_real, delay_sender_signal_two, '.-', label='Sender', color='xkcd:light orange')
#axs2[1][1].plot(delay_receiver_X_real, delay_receiver_signal_two, '.-', label='Receiver', color='xkcd:orange')
#axs2[1][1].set_xlabel('Real time (ms)')
#axs2[1][1].tick_params(labelbottom=True, labelleft=True)
#axs2[1][1].legend()
#axs2[1][1].grid(True)

axs2[2].remove()
#axs2[2][1].remove()

ax2 = fig2.add_subplot(gs2[2,:], sharex = axs2[0])
ax2.plot(expected_X_real, expected_total, '.-', label='Expected Total', color='xkcd:light grey')
ax2.plot(receiver_X_real, receiver_total, '.-', label='Total = Signal 1 - Signal 2', color='xkcd:cerulean')
#ax2.plot(delay_receiver_X_real, delay_receiver_total, '.-', label='Total delayed', color='xkcd:orange')
ax2.set_xlabel('Real time (ms)')
ax2.legend()
ax2.grid(True, which='both')
ax2.grid(which='minor', lw=0.4, ls='--')

plt.show()