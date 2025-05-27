### This directory contains benchmarks ported to Timed Rebeca.

## Converting LF to Timed Rebeca

We use snippets from `Railroad.lf` and `Raft.lf` as examples.

### Step 1: Create a Rebeca environment variable for each `#define` in LF preamble

For example,
```
preamble {=
  #define MSG_HEART       -1
  #define MSG_ACK         -2
  #define MSG_ELECTION    -3
  #define MSG_VOTE        -4
  #define MSG_LEADER      -5

  #define TIMEOUT_MS_LOW  150
  #define TIMEOUT_MS_HIGH 300
=}
```
becomes 
```
env int MSG_HEART       = -1;
env int MSG_ACK         = -2;
env int MSG_ELECTION    = -3;
env int MSG_VOTE        = -4;
env int MSG_LEADER      = -5;

env int TIMEOUT_MS_LOW  = 150;
env int TIMEOUT_MS_HIGH = 300;
```
Environment variables must live outside of `reactiveclass`.

### Step 2: Create a `reactiveclass` for each reactor. Set the default buffer size to 10.

For example, `reactor Train {...}` becomes 
```
reactiveclass Train(10) {
    ...
}
```

### Step 3: If the reactor has output ports that connects to some downstream reactors.
Declare the class type of the downstream reactor in `knownrebecs`. This assumes
that each output port of the upstream reactor only connects to one class type
of downstream reactor.
   
For example, the `Train` reactor connects to `Controller`. Thus in `Train`'s
`reactiveclass` we have
```
    // Put downstream reactors here.
    knownrebecs {
        Controller controller;
    }
```

### Step 4: Encode LF's state variables, ports, and actions in Rebeca's `statevars` block.

For example, the `Train` reactor has two state variables. Reproduce them
verbatim here. 

For each output port, create a `_value` field with the same data
type (assuming Rebeca has it). 

For each input port, create a `_value` and a
`_is_present` field. 

For an action, do the same. If the action is pure (i.e.,
untyped), only the `_is_present` field is needed.

If a reaction has multiple triggers, an auxiliary boolean variable named
`<reaction_name>_scheduled` needs to be declared.

Here is a concrete example of the `Train` reactor:
```
    statevars {
        // State variables
        int _out;
        int _mode;
        // Outputs (only _value field needed)
        int out_value;
        // Inputs (both _value and _is_present fields needed)
        int signal_value;
        boolean signal_is_present;
        // Actions
        boolean outUpdated_is_present;
        boolean toModeAway_is_present;
        boolean toModeWait_is_present;
        boolean toModeBridge_is_present;

        // In this example, there is no reaction triggered by multiple inputs. But if there is (assuming reaction 1 is), we need the following line.
        // boolean reaction_1_scheduled;
    }
```

If there are variables of C structs (defined in a preamble), flatten the struct by
explicitly declaring the struct members and Rebeca variables. For example,
```LF
preamble {=
  typedef struct msg_t {
      int payload;
      int term;
  } msg_t;
=}

...

reactor RaftNode {
    input[num_nodes] in: msg_t
}
```
This becomes
```rebeca
reactiveclass RaftNode(10) {
    statevars {
        int in_0_payload;
        int in_0_term;
        int in_1_payload;
        int in_1_term;
        int in_2_payload;
        int in_2_term;
        ...
        int in_{num_nodes}_payload;
        int in_{num_nodes}_term;
    }
}
```
These state variables all need to be initiated in the constructor.

### Step 5: In the constructor, initialize the state variables (if specified in the LF program) and schedule reactions triggered by `startup` and timers.

In the LF program, state variables are given initial values.
```
    state _out:int = 0
    state _mode:int = 0  // 0 = AWAY, 1 = WAIT, 2 = BRIDGE
```
Thus in Rebeca's `Train` constructor, we  need to initialize them.
```
    Train() {
        _out = 0;
        _mode = 0;
    }
```
Since `Train`'s reaction 1 is triggered by `startup` and reaction 2 is triggered
by a timer with an offset of `1 nsec` and a period of `1 sec`, we need to
schedule these reactions' first invocations.

`_is_present` variables and `_scheduled` variables should be initialized
to `false` in the constructor.

```
    Train() {
        _out = 0;
        _mode = 0;
        self.reaction_1(); // Schedule the startup reaction.
        self.reaction_2() after(1); // Schedule the first timer with an offset of 1 nsec.

        // Setting the _is_present variables to false.
        signal_is_present = false;
        outUpdated_is_present = false;
        toModeAway_is_present = false;
        toModeWait_is_present = false;
        toModeBridge_is_present = false;

        // In this example, there is no reaction triggered by multiple inputs. But if there is (assuming reaction 1 is), we need the following line.
        // reaction_1_scheduled = false;
    }
```

### Step 6: Encode methods.

For each LF method, create a local method in Rebeca. For example,
here is an LF method.
```LF
method max(i: int, j: int): int {=
    if (i > j) return i;
    else return j;
=}
```

In Rebeca,
```rebeca
int max(int i, int j) {
    if (i > j) return i;
    else return j;
}
```

For each statement and expression in C, use the following table for conversion.

| Stmt / Expr Name | C | Rebeca |
| :---------------- | :------ | :---- |
| If statement        |   `if (...) {...} else if {...} else {...}`  | Same as C |
| Arithmetic comparison | `> \| < \| >= \| <= \| == \| !=` | Same as C
| Assignment | `<var1> = <var2>` | Same as C |
| LF state variables | `self.<var>` | `<var>` |
| Setting a port | `lf_set(<upstream_output_port>, <value>);` | `<upstream_output_port>_value = <value>;` Then for each downstream reaction triggered, insert `<downstream_reactor_instance_name>.read_port_<downstream_input_port>(<upstream_output_port>_value) after (after_delay_along_connection);` |
| Scheduling an action | `lf_schedule(<action>, <additional_delay>);` | `self.lf_schedule_<action>() after(<min_delay> + <additional_delay>);` |

### Step 7: Encode reactions.

For each reaction, create a `msgsrv`.
```
    msgsrv <reaction_name>() {
        ...
    }
```

At the end of each reaction `msgsrv`, a postamble is needed based on the
following conditions.

| Condition | Postamble | Rebeca | 
| :---------------- | :------ | :------ |
| The reaction is timer-driven. | Schedule the next timer-driven invocation. | `self.<reaction_name>() after(<timer_period>);` |
| The reaction is triggered by inputs or actions. | Schedule a postamble msgsrv. If the reaction has a `_scheduled` variable, it also needs to be set to `false` before calling the postamble. | `<reaction_name>_scheduled = false;` `self.<reaction_name>_postamble();` |

The reason why `<reaction_name>_scheduled = false;` is called inside the reaction body is that the postamble has the largest `@globalPriority`. If `<reaction_name>_scheduled = false;` is inside the postamble, then `<reaction_name>_scheduled` might remain `true` for a falsely long time, preventing other correct instances of `<reaction_name>` from being scheduled. (A concrete example is reaction 3 in `Raft.lf`. It needs to be scheduled in time to renew the election timeout.)

Then, assign a `@globalPriority(x)` to the message server with `x` being _twice_ the
"level" of the message server in the Rebeca program and accounting for the
auxiliary message servers in the level calculation.

As an end-to-end example, here is reaction 2 in `Railroad.lf`:
```C
    // This reaction has level 3, for example.
    reaction(t) -> out, outUpdated {=
        if (self->_mode == 0) {
            lf_set(out, 0);
            self->_out = 0;
            lf_schedule(outUpdated, 0);
        }
        else if (self->_mode == 2) {
            lf_set(out, 1);
            self->_out = 1;
            lf_schedule(outUpdated, 0);
        }
    =}
```
In Rebeca, this becomes:
```C
    @globalPriority(6) // 6 = 2 * 3
    msgsrv reaction_2() {
        if (_mode == 0) {
            out_value = 0;
            controller.read_port_out_w(out_value);
            _out = 0;
               self.lf_schedule_outUpdated() after(0+0);
        }
        else if (_mode == 2) {
            out_value = 1;
            controller.read_port_out_w(out_value);
            _out = 1;
            self.lf_schedule_outUpdated(0) after(0+0);
        }
        // First set _scheduled varibale to false
        reaction_2_scheduled = false;
        // Postamble: schedule the next timer-driven invocation.
        self.reaction_2() after(1000000000);
        // This reaction is not triggered by input ports or actions. But if it is, use the following line.
        // self.reaction_2_postamble();
    }
```

### Step 8: Create auxiliary message servers.

#### Reaction postamble

Each reaction needs to have a postamble `msgsrv` for resetting
variables in the Rebeca encoding. Inside the `msgsrv`, the `_is_present`
fields of input ports and actions triggering the reaction needs to be
set to `false`.

The postamble `msgsrv` must have a higher `@globalPriority` than all reactions in
the same reactiveclass, so that all concurrent reactions can finish firing before resetting triggers.

Here is an example from the `Sink` reactor in `TrainDoorFeedback`.
```
@globalPriority(5)
msgsrv reaction_1_postamble() {
    in1_is_present = false;
    in2_is_present = false;
    in3_is_present = false;
}
```

#### Scheduling actions

For each action available, create a `msgsrv` named `lf_schedule_<action>`.
Inside the `msgsrv`, set the action's `_is_present` field to `true` and invoke
the reactions triggered by this action.

For example, the `outUpdated` action in the `Train` reactor produces the
following code:
```
    msgsrv lf_schedule_outUpdated() {
        outUpdated_is_present = true;
        self.reaction_3();
    }
```
A `lf_schedule_` message server should carry a `@globalPriority` of the global priority of the reaction triggered by the action - 1.

#### Actions with `forever` minimum spacing and `update` policy

When an action has a minimum spacing of `forever` and a policy of `update`,
and the action is scheduled multiple times, the earlier instances of the action
must be dropped. Only the latest action instance is kept. To model this in Rebeca,
a `_pending_count` variable is introduced. When the action is scheduled
in a reaction, the count is incremented. When the `lf_schedule` message server
executes, decrement the count and check if the count equals 0. Only further
trigger reaction message servers when the count equals 0.

An example can be found in `Raft.lf` with an `election_timeout_reached` action.
When the action is scheduled, increment the count.
```
msgsrv reaction_2() {
    // Select election timeout
    self.random_int_in_range();
    
    // Track the number of lf_schedule_election_timeout_reached msgsrv enqueued.
    election_timeout_reached_pending_count++;
    // Schedule lf_schedule_election_timeout_reached.
    self.lf_schedule_election_timeout_reached() after(election_timeout);
}
```

When the `lf_schedule` message server executes, decrement the count and check
if it equals 0.
```
// Check if the action is updated. If so, cancel the current one.
msgsrv lf_schedule_election_timeout_reached() {
    // Since this msgsrv is triggered, decrement the count.
    election_timeout_reached_pending_count--;
    // Check if any is still pending, if so, don't do anything,
    // i.e., canceling the stale event. Only trigger reactions
    // when there are no actions pending. 
    if (election_timeout_reached_pending_count == 0) {
        election_timeout_reached_is_present = true;
        // ... Trigger a reaction based on mode ...
    }
}
```

#### Reading input ports

For each input port the reactor has, create a `msgsrv` named
`read_port_<port_name>` with one parameter with the same type as the input
port's type. Inside the `msgsrv`, set the `_value` field and the `_is_present`
field, then invoke the reaction triggered by this port.

For example, the `signal` input port in the `Train` reactor produces the
following code:
```C
    msgsrv read_port_signal(int _signal_value) {
        signal_value = _signal_value;
        signal_is_present = true;
        self.reaction_3(); // Invoke the reaction triggered by this port.
    }
```
A `read_port_` message server should carry a `@globalPriority` of the
global priority of the reaction triggered by the input port - 1.

If multiple triggers (input ports and actions) trigger the same
reaction, in each of the `read_port_` and `lf_schedule_` auxiliary
message server, the following logic needs to be added to prevent
scheduling the same reaction multiple times.
```
// Do not re-schedule reaction_1 if one of
// the input ports has scheduled it already.
if (!reaction_1_scheduled) {
    self.reaction_1();
    reaction_1_scheduled = true;
}
```

A full example including the above logic is `read_port_in1` message
server in `TrainDoorFeedback`.
```
@globalPriority(3)
msgsrv read_port_in1(int _in1_value) {
    // Register the input port value.
    in1_value = _in1_value;
    in1_is_present = true;
    
    // Do not re-schedule reaction_1 if one of
    // the input ports has scheduled it already.
    if (!reaction_1_scheduled) {
        self.reaction_1();
        reaction_1_scheduled = true;
    }
}
```

### Step 9: In the `main` block, instantiate each `reactiveclass` based on the main reactor.

For each reactor instance in LF's main reactor, instantiate the corresponding `reactiveclass` and put downstream reactor instances' names in the second
parantheses.

For example, here is LF's main reactor:
```
main reactor {
    train_w = new Train()
    train_e = new Train()
    controller = new Controller()
    train_w.out -> controller.out_w
    train_e.out -> controller.out_e
    controller.signal_w -> train_w.signal
    controller.signal_e -> train_e.signal
}
```
Here is the Rebeca version:
```
main {
    Train train_w(controller):();
    Train train_e(controller):();
    Controller controller(train_w, train_e):();
}
```
