### This directory contains benchmarks ported to Timed Rebeca.

## Converting LF to Timed Rebeca

We use `Railroad.lf` for example.

### Step 1: Create a `reactiveclass` for each reactor. Set the default buffer size to 10.

For example, `reactor Train {...}` becomes 
```
reactiveclass Train(10) {
    ...
}
```

### Step 2: If the reactor has output ports that connects to some downstream reactors.
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

### Step 3: Encode LF's state variables, ports, and actions in Rebeca's `statevars` block.

For example, the `Train` reactor has two state variables. Reproduce them
verbatim here. For each output port, create a `_value` field with the same data
type (assuming Rebeca has it). For each input port, create a `_value` and a
`_is_present` field. For an action, do the same. If the action is pure (i.e.,
untyped), only the `_is_present` field is needed.
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
    }
```

### Step 4: In the constructor, initialize the state variables (if specified in the LF program) and schedule reactions triggered by `startup` and timers.

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
```
    Train() {
        _out = 0;
        _mode = 0;
        self.reaction_1(); // Schedule the startup reaction.
        self.reaction_2() after(1); // Schedule the first timer with an offset of 1 nsec.
    }
```

### Step 5: Encode reactions.

For each reaction, create a `msgsrv`.
```
    msgsrv <reaction_name>() {
        ...
    }
```

For each statement and expression in C, use the following table for conversion.

| Stmt / Expr Name | C | Rebeca |
| :---------------- | :------ | :---- |
| If statement        |   `if (...) {...} else if {...} else {...}`  | Same as C |
| Arithmetic comparison | `> \| < \| >= \| <= \| == \| !=` | Same as C
| Assignment | `<var1> = <var2>` | Same as C |
| LF state variables | `self.<var>` | `<var>` |
| Setting a port | `lf_set(<upstream_output_port>, <value>);` | `<upstream_output_port>_value = <value>; <downstream_reactor_instance_name>.input_<downstream_input_port>_reads(<upstream_output_port>_value);` |
| Scheduling an action | `lf_schedule(<action>, <additional_delay>);` | `self.lf_schedule_<action>() after(<min_delay> + <additional_delay>);` |

At the end of each reaction `msgsrv`, a postamble is needed based on the
following conditions.

| Condition | Postamble | Rebeca | 
| :---------------- | :------ | :------ |
| The reaction is timer-driven. | Schedule the next timer-driven invocation. | `self.<reaction_name>() after(<timer_period>);` |
| The reaction is triggered by inputs or actions. | Clear the `_is_present` field for each input or action. | `<input/action>_is_present = false;` |

As an end-to-end example, here is reaction 2 in `Railroad.lf`:
```C
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
    msgsrv reaction_2() {
        if (_mode == 0) {
            out_value = 0;
            controller.input_out_w_reads(out_value);
            _out = 0;
               self.lf_schedule_outUpdated() after(0+0);
        }
        else if (_mode == 2) {
            out_value = 1;
            controller.input_out_w_reads(out_value);
            _out = 1;
            self.lf_schedule_outUpdated(0) after(0+0);
        }
        // Postamble: schedule the next timer-driven invocation.
        self.reaction_2() after(1000000000);
    }
```

### Step 6: Create auxiliary message servers for action scheduling and input port reading.

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

Then for each input port the reactor has, create a `msgsrv` named
`input_<port_name>_reads` with one parameter with the same type as the input
port's type. Inside the `msgsrv`, set the `_value` field and the `_is_present`
field, then invoke the reaction triggered by this port.

For example, the `signal` input port in the `Train` reactor produces the
following code:
```C
    msgsrv input_signal_reads(int _signal_value) {
        signal_value = _signal_value;
        signal_is_present = true;
        self.reaction_3(); // Invoke the reaction triggered by this port.
    }
```

### Step 7: In the `main` block, instantiate each `reactiveclass` based on the main reactor.

For each reactor instance in LF's main reactor, instantiate a `reactiveclass`
with `@priority(1)` and put downstream reactor instances' names in the second
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
    @priority(1) Train train_w(controller):();
    @priority(1) Train train_e(controller):();
    @priority(1) Controller controller(train_w, train_e):();
}
```
