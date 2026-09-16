# StopBath Flipper Remote

A specification for handoff to a coding agent.

A small physical control surface for a StopBath appliance, so the photographer
can present, start, and terminate a session without taking a phone out.

This document is a companion to `STOPBATH_SPEC.md` and does not replace any rule
in it. Where the two disagree, the main specification wins and the disagreement
is a defect to raise, not a choice to make.

## How to read this document

This document is linear. Read it top to bottom once before doing anything, then
work the stages in order.

Every normative line carries a tag. Do not treat an untagged sentence as a
requirement.

| Tag | Meaning |
|---|---|
| **MUST** | Mandatory. Not negotiable. Failing it fails the phase. |
| **MUST NOT** | Prohibited. Not negotiable. |
| **DECIDE** | A blocking decision belonging to the author. Numbered `FD1` to `FD19`. Stop and ask. Never guess. |
| **GUIDANCE** | A suggestion with reasoning. You may depart from it. Say so and say why. |

Identifiers follow one scheme across all three documents. The second letter says
what kind of thing it is, and the first says which document owns it.

| Prefix | Meaning |
|---|---|
| `E1` and up | an execute phase in `STOPBATH_SPEC.md` |
| `D1` and up | a decision in `STOPBATH_SPEC.md` |
| `FE1` and up | an execute phase in this document |
| `FD1` and up | a decision in this document |
| `PE1` and up | an execute phase in `STOPBATH_PERIPHERAL_EXTENSION.md` |
| `PD1` and up | a decision in that document |

An earlier draft numbered the phases here `F1` and up, which shares a leading
letter with `FD` and produced two unrelated things both called five. **MUST NOT**
reintroduce a phase prefix that is a prefix of a decision prefix.

Concrete values are illustrative unless they appear in **Appendix A**, which
lists values that are unverified and must be settled during the Plan stage.

### What this document is not

This specification was written without reading the Flipper Zero SDK, the Flipper
RPC definitions, or any Flipper application source. It contains no verified claim
about what the firmware provides, which transports an application may use, how
much heap an application may take, or how QR rendering performs on the device.

Every such claim is deferred to the Plan stage. Where this document names a
Flipper capability, treat it as a question to answer, never as a fact to build
on.

---

# Part 0: Absolute rules

These are the same rules as Part 0 of `STOPBATH_SPEC.md`, restated because this
is a separate repository worked in separate sessions. Where a rule needs to differ
because the language is C rather than Go, that is called out.

## 0.1 Version control

**MUST NOT** perform any git write operation, ever, under any circumstance, in
any session, regardless of later instruction. This includes `add`, `commit`,
`push`, `tag`, `merge`, `rebase`, `reset`, `revert`, `cherry-pick`, `stash`,
branch creation or deletion, any history mutation, any write to `.git`, and any
wrapper, alias, script, hook, or editor integration that performs one indirectly.

**MUST NOT** ask for permission to commit. The answer is permanently no.

**MAY** run read-only git commands: `status`, `diff`, `log`, `ls-remote`.

The author performs every commit personally. You may be asked to propose commit
message text and should supply it as plain text in your reply. Proposing text
never authorises creating the commit.

## 0.2 Evidence before code

**MUST** read the real SDK, header, or official document before writing anything
that depends on it.

**MUST** record exact firmware version, API version, SDK tag or commit, and
toolchain version for everything evaluated.

**MUST NOT** invent a Flipper API, a furi function signature, an RPC message, a
GPIO capability, a display primitive, a build flag, or a filesystem path.

**MUST NOT** treat a forum post, a search snippet, a previous message, or your
own earlier summary as ground truth for an API.

## 0.3 Blocked means ask

When behaviour is ambiguous, underspecified, hardware-dependent, or a judgement
call, **MUST** stop that phase and ask.

**MUST** phrase the question with all five of:

1. the exact decision that is blocked
2. why the available evidence does not settle it
3. the viable options
4. the meaningful trade-offs
5. your recommendation, explicitly labelled as a recommendation

**MUST** batch questions by stage rather than sending them one at a time.

## 0.4 Tests first

**MUST** write a failing test describing required behaviour, confirm it fails for
the expected reason, implement the smallest correct change, run focused tests,
then run the full suite, then refactor only while green.

**MUST NOT** modify, weaken, skip, or delete a test written in an earlier phase to
make a later phase pass. If a later phase genuinely invalidates an earlier test,
stop and raise it, naming the test, the behaviour that changed, and why.

Protocol parsing, event encoding, QR encoding, and state handling are all pure
logic and **MUST** be testable on a development machine without a Flipper
attached.

## 0.5 Naming

**MUST** use verbose, self-documenting names, including in C where the
surrounding SDK convention is terser.

**MUST NOT** use `mgr`, `svc`, `util`, `helper`, `data`, `item`, `tmp`, `buf`,
`msg`, `cb`, `ctx` beyond an SDK-imposed signature, `handle`, `process`, `run`,
or `execute` as a name.

**MUST NOT** use a single-letter name except a loop index.

**MUST NOT** rely on an implicit or terse callback parameter. Where the SDK
requires a `void` context pointer, name the cast local descriptively, for example
`remoteApplicationState`, never `state` or `s`.

**MAY** use `id`, `url`, `len` where an SDK signature imposes it, and **MUST**
name the local variable descriptively at the first opportunity.

## 0.6 Comments

**MUST** explain why: why a bound exists, why a message is rejected, why an
allocation is sized as it is, why a reconnect discards state.

**MUST NOT** restate syntax or paraphrase a descriptive name.

## 0.7 Do not repeat yourself

**MUST** express a protocol verb, a bound, a timeout, or a display string in
exactly one place.

**MUST** share test fixtures rather than reimplementing them per test file.

**MUST NOT** duplicate the protocol grammar between the parser and the encoder.
Derive one from a single table.

## 0.8 No em dashes

**MUST NOT** use the em dash character in any tracked text file: source, comments,
test names, test data, assertion messages, logs, documentation, proposed commit
message text, or user-facing strings.

**MUST** add a repository scan to continuous integration that fails when an em
dash is present in a tracked text file.

Note for this repository specifically: the draft this specification was derived
from had been through a document converter that turned em dashes into triple
hyphens and en dashes into double hyphens. **MUST** scan for those sequences in
prose as well, since they are the same defect wearing a different coat.

## 0.9 No placeholders

**MUST NOT** emit a command, script, or configuration line containing a
placeholder such as an angle-bracketed path, identifier, or filename.

If a real value is needed and not yet known, **MUST** stop, ask for the output of
the preceding command, and then produce the command with the real value in place.

## 0.10 C specific rules

The language is C, because that is what Flipper applications are written in. See
`FD1`.

**MUST** bound every read from the peripheral connection by an explicit maximum
message length, checked before any copy.

**MUST** treat every byte arriving over the connection as hostile input, and parse
it with a state machine that cannot advance past its buffer.

**MUST NOT** use `strcpy`, `strcat`, `sprintf`, `gets`, or any unbounded copy.

**MUST NOT** allocate in the input path. Size buffers once at application start.

**MUST** check every allocation and every SDK call that can fail, and surface the
failure rather than continuing.

**MUST** free every furi resource acquired, on every exit path including error
paths.

**MUST** compile warning clean, with warnings as errors, at the strictest setting
the toolchain and SDK permit.

**MUST** run the pure logic under a sanitiser build on the development machine,
covering at minimum address and undefined behaviour.

**GUIDANCE** Fuzz the protocol parser. It is the only untrusted input surface in
the application and it is small enough to fuzz thoroughly.

## 0.11 Contracts are sacred once accepted

**MUST NOT** rename, alias, normalise, reinterpret, or silently improve an
accepted protocol verb, event name, status code, or error code.

**MUST NOT** supply a silent default for a required field.

**MUST NOT** accept unknown fields.

**MUST** version the protocol and reject an incompatible peer explicitly.

**MUST** document every unavoidable deviation in `IMPLEMENTATION_DEVIATIONS.md`
with a one-line justification and a covering test.

## 0.12 Dependencies

Before adding one, **MUST** record: the behaviour it provides, why the SDK is
insufficient, maintenance status, licence, size on device, security implications,
test strategy, and replacement cost.

**GUIDANCE** A QR encoder is the only dependency this application obviously needs.
Prefer a small, permissively licensed, widely reviewed C implementation over
writing one, and prefer one with no dynamic allocation.

## 0.13 Consolidated prohibitions

Reread this list at the start of every phase. It adds nothing new.

Never:

1. Perform a git write operation.
2. Invent a Flipper API, RPC message, capability, or path.
3. Guess a **DECIDE** item.
4. Write implementation before a failing test.
5. Weaken or delete an earlier phase's test.
6. Encode StopBath domain meaning in the Flipper. See 2.1.
7. Hold authoritative session state on the Flipper.
8. Queue a button event across a disconnection and replay it later.
9. Use an unbounded copy, or allocate in the input path.
10. Accept a message without a length bound and a version check.
11. Expose a shell, an arbitrary command, or a file path from the protocol.
12. Persist a Wi-Fi passphrase, guest token, or session identifier to Flipper
    storage.
13. Use an em dash, or a converter-mangled double or triple hyphen, in a tracked
    text file.
14. Emit a placeholder in a command or script.
15. Use a single-letter name outside a loop index.
16. Turn the peripheral protocol into a second administration interface.
17. Declare a phase complete on the strength of your own environment alone.

---

# Part 1: Intent and scope

## 1.1 What it is

The Flipper Zero acts as a small, low power physical interface to StopBath
during a shoot. It exists so the photographer can perform the handful of actions
the normal workflow needs without taking a phone out:

present, join, pocket, shoot, terminate, next subject.

The Flipper is not StopBath. It is a peripheral. StopBath remains authoritative
for sessions, networking, guest state, photographs, delivery state, and the
interpretation of every user action.

The first implementation is deliberately a prototype of the physical interaction
model. If it works, the same concepts may inform a dedicated remote built on a
microcontroller with an e-ink display. See Part 9.

## 1.2 Goals

The first useful version:

- connects physically to the Raspberry Pi running StopBath
- receives display state from StopBath
- renders QR codes from payloads supplied by StopBath
- shows minimal human readable status
- reports button events to StopBath
- allows left and right navigation between the Wi-Fi QR and the guest URL QR
- allows a short centre press to request a new session
- allows a deliberate long centre press to request termination
- works with no internet connectivity
- keeps a phone out of the photographer's operational workflow
- keeps all meaningful logic and session state in StopBath

## 1.3 Non-goals

**MUST NOT**: manage Wi-Fi; create or destroy sessions independently; store or
receive photographs; implement the StopBath session state machine; decide
whether an action is valid in the current state; become a second administration
interface; require cloud connectivity; require a phone; persist sensitive state;
duplicate StopBath logic; or become a general purpose remote control framework
in the first version.

**MUST NOT** become required. A StopBath appliance with no Flipper attached
**MUST** remain fully operable through the dashboard, unchanged. The Flipper is a
convenience, never a dependency, and never the only path to a destructive action.

## 1.4 Radical ownership

A user **MUST** be able to build the application themselves, read its source,
change the controls, change the interface, implement the peripheral protocol on
different hardware, and replace the Flipper entirely.

The Flipper is a convenient implementation of the interface, not a required
proprietary accessory. If someone builds a better remote, that is a success.

---

# Part 2: The peripheral contract

## 2.1 The ownership principle

The Flipper reports what physically happened. StopBath decides what it means.

```text
Flipper                         StopBath
   |                               |
   | CENTER_LONG . . . . . . . . .>|
   |                               |
   |                         interpret event
   |                         against current state
   |                               |
   |<. . . . . . DISPLAY or STATUS |
   |                               |
```

**MUST NOT** encode anything equivalent to a mapping from `CENTER_LONG` to
terminate. The Flipper emits `CENTER_LONG` and nothing more.

StopBath may currently interpret that as a termination request. That
interpretation belongs to StopBath, and changing it **MUST NOT** require new
Flipper firmware.

This is the same separation the main specification applies to its adapters, and
it is the single most important rule in this document.

## 2.2 Responsibilities

StopBath owns session lifecycle, session identifiers, guest state, network
configuration, QR payload contents, interpretation of button events, validation
of requested actions, photo ingestion and delivery, persistence, security policy,
recovery behaviour, and authoritative application state.

The Flipper owns physical button detection, reporting raw semantic button events,
rendering QR codes, rendering minimal status, switching the visible page when the
contract allows it, connection status indication, and display lifecycle.

## 2.3 Controls

The initial interaction surface is deliberately small.

| Input | Handled | Event reported |
|---|---|---|
| Centre short press | by StopBath | `CENTER_SHORT` |
| Centre long press | by StopBath | `CENTER_LONG` |
| Left short press | by StopBath | `LEFT_SHORT` |
| Right short press | by StopBath | `RIGHT_SHORT` |
| Back short press | by StopBath | `BACK_SHORT` |
| Down short press | locally | none, see 2.4 |
| Down long press | locally | none, see 2.4 |
| Up | unused | none |

The down button is the one control the Flipper acts on itself rather than
reporting, because it concerns the device's own screen and not StopBath's domain.
That is presentation state, which 2.5 already allows the Flipper to own.

**MUST NOT** extend local handling beyond the screen lock. Every other button
reports and nothing more.

**MUST NOT** let `BACK_SHORT` perform or request a destructive action under any
interpretation.

The current StopBath interpretation, which lives in StopBath and is a product
hypothesis rather than protocol semantics:

| Event | Interpretation |
|---|---|
| `CENTER_SHORT` | request new session |
| `CENTER_LONG` | request termination of the current session |
| `LEFT_SHORT` | previous display page, being the guest Wi-Fi QR |
| `RIGHT_SHORT` | next display page, being the guest gallery address |

This mapping is the fixed shape of the product. Additional verbs and additional
display states are acceptable. Changing what these four events mean is not, and
**MUST** be raised with the author rather than adjusted.

## 2.4 Screen lock and accidental activation

StopBath sessions last minutes, and the normal workflow is to terminate a session
and then pocket the device, not to carry a running session around. The risk of a
pocket ending a live session is therefore small, and the guard is sized to match
rather than built for a scenario the product does not have.

The guard is one rule:

**MUST NOT** transmit any button event while the screen is locked, the display is
off, the application is backgrounded, or another application is running. A press
in any of those conditions produces nothing.

### Locking

A short down press locks the screen. A long down press unlocks it.

**MUST** handle both locally and **MUST NOT** transmit them as events. The lock
concerns the device's screen, not StopBath's state, and a locked device that has
to ask the appliance for permission to unlock is a device that stops working when
the cable is out.

**MUST** make the locked state obvious on the display, so the photographer can see
at a glance whether a press will do anything.

**MUST** report the lock state to StopBath as status, so the dashboard can show
that the remote is locked. Reporting state is not the same as asking permission.

**MUST** require a deliberate long press to unlock, so the unlock is not itself
pocket-triggerable.

The firmware may already provide a device level lock. **MUST** check before
building one, see `FD19`, and prefer the firmware's own if it can be entered and
left from within the application.

### Appliance side

**MUST** have StopBath, not the Flipper, make the final decision on a termination
request, using the foreground and lock flags carried on the event. A peripheral
that self-certifies its own guard is trusting the untrusted side.

**MUST** surface every termination on the dashboard with its origin recorded, so a
session ended from the Flipper is distinguishable afterwards from one ended in
the browser. If accidental activation ever does become a problem, this is the only
evidence that will exist.

**MUST NOT** add a confirmation step in the first version. A long press plus the
lock is the design. Whether more is needed is `FD5` and is a field question.

## 2.5 State ownership

The appliance owns the current page. The Flipper renders what it was last sent
and nothing more.

An earlier draft said the Flipper may hold presentation state such as which page
is visible. That contradicted the control table in 2.3, where left and right are
reported to StopBath rather than handled locally, and it is withdrawn. The only
thing the Flipper decides for itself is the screen lock in 2.4.

**MUST NOT** hold a belief about whether a session is active, whether a guest is
connected, what the session identifier is, or which page should be visible.

**MUST** render the last state record received, and **MUST** replace it wholly
when a new one arrives rather than merging fields.

**MUST** show a clear not connected indication rather than stale content if the
link drops, since the Flipper cannot know whether what it holds is still true.

The cost of this is one round trip per page change. The benefit is that reconnect
is a single full state record with nothing to reconcile, and that the dashboard
can show which page the photographer is presenting.

## 2.6 Display

The display is small, and that constraint is useful. Show only what the
photographer's immediate task requires.

Conceptual states, to be laid out properly against the real display in `FD3`:

```text
STOPBATH          STOPBATH          STOPBATH
                                    
Ready             [ QR CODE ]       Guest connected
                                    
[OK] New session  Wi-Fi             Ready to shoot
                  <  1/2  >
```

```text
STOPBATH          STOPBATH
                  
3 photos          Pi disconnected
delivered         
                  Reconnect USB
```

**MUST** prefer sending a semantic status code over arbitrary display text, so
the Flipper owns presentation without owning meaning. Send a status code such as
`GUEST_CONNECTED`, not a sentence to print.

**MUST** treat readability in daylight, at arm's length, by a stranger, as a
requirement rather than a nicety. The person scanning is not the person holding
the device.

## 2.7 Presentation payloads: QR and NFC

Each display page presents the same payload two ways at once: a QR code on the
screen and an NFC target the guest can tap. The photographer does not choose
between them and does not need to know which phone the guest is holding.

### Why both, and why it is not simply one per platform

The obvious framing is QR for iPhone and tap for Android. That is right for one
page and wrong for the other, and the reason matters.

- **Wi-Fi joining.** Android reads Wi-Fi credentials from an NFC tag natively and
  offers to join. iOS does not support Wi-Fi credentials over NFC without a
  dedicated application, so an iPhone must join by QR. The split is real and the
  Wi-Fi page needs both surfaces to cover both platforms.
- **Opening a URL.** Both platforms read an NDEF URI record. Recent iPhones do it
  in the background with the screen unlocked. So on the guest URL page, both
  surfaces work on both platforms and the second one is redundancy rather than
  coverage.

That redundancy is the point of the guest URL page. Captive portal auto-opening
is unreliable, which the main specification already accepts and plans a fallback
for. A tap that lands the guest directly on the gallery is a far better fallback
than reading a local address aloud.

### Page model

| Page | QR carries | NFC carries | Covers |
|---|---|---|---|
| 1, Wi-Fi | Wi-Fi credentials for this session | Wi-Fi credential record | iPhone by QR, Android by tap |
| 2, Guest URL | the guest gallery address | URI record for the same address | both platforms by either, and the captive portal fallback |

**MUST** keep the two pages in this order, so the first thing presented is the one
that gets the guest onto the network.

**MUST NOT** require the guest to use a particular surface. Whichever works on
their phone is correct.

### QR

**MUST** send the QR payload, not a rendered bitmap. The Flipper generates and
renders the matrix locally.

Less data crosses the connection, the protocol stays independent of any display
representation, StopBath needs no Flipper specific rendering, and a future
device can render the same payload differently.

Size constraint, from the display geometry: a version 1 QR is 21 by 21 modules,
and a four module quiet zone on each side gives 29 by 29. At two display pixels
per module that is 58 by 58 pixels, which fits the display height. This arithmetic
is sound but the operational conclusion is not, so:

**MUST** keep guest URLs short enough to stay at a low QR version.

**MUST NOT** add query parameters or verbose payloads to a QR intended for this
display.

**MUST** establish scanning reliability by physical testing with a representative
range of real phones, under realistic lighting and viewing distance.

**MUST NOT** treat a QR that renders as a QR that scans.

The Wi-Fi QR carries an SSID, a security type, and a passphrase, so it will be
substantially larger than a short guest URL and may not be viable at this size.
That is `FD4`. If it is not viable, the phone remains the Wi-Fi joining surface
and the Flipper carries the guest URL only, which is still a useful product.

Note the interaction with the main specification: StopBath generates a fresh
SSID and passphrase per session, and both feed the Wi-Fi QR. Shortening either to
fit this display is a change to the main specification's credential grammar and
**MUST** be raised there rather than decided here.

### NFC

**MUST** send the NFC payload as data, not as an encoded tag image, on the same
principle as the QR payload. The Flipper builds the NDEF record locally.

**MUST** derive the QR payload and the NFC payload for a page from one value sent
once. Sending the same address twice in two encodings is a duplication defect and
a chance for the two to disagree.

Unverified and blocking, all of it:

- whether the firmware lets an application emulate an NDEF target with a payload
  chosen at runtime, rather than replaying a saved tag. See `FD15`
- whether an application can render a QR on screen and present an NFC target at
  the same time, or whether NFC emulation takes over the display. See `FD16`
- whether Wi-Fi credential records are reproducible on this hardware in a form
  Android accepts. Independent reports describe this record type as awkward to
  emulate correctly at the protocol layer, so treat it as a real risk. See `FD17`

**MUST** degrade rather than fail if any of the above turns out to be
unavailable. The QR surface alone is a working product, and the first prototype
in Part 7 does not depend on NFC.

**MUST NOT** duplicate an NFC capability that the StopBath appliance already
provides, if one is later added there.

### Ergonomics

Tapping requires a guest to bring their phone within a few centimetres of the
device, which is a different physical act from holding a screen out to be
photographed. **MUST** record in field notes whether presenting for a tap is
comfortable for both people, and whether guests understand which part of the
device to tap without being told.

## 2.8 Protocol

The protocol **MUST** be small, versioned, explicit, resistant to malformed
input, reconnectable, and human debuggable where practical.

The protocol definition **MUST** live in the main StopBath repository, because
StopBath owns the peripheral contract. This repository implements that contract.

Ownership is not the same as build order. This peripheral is built before the
appliance side exists, which is deliberate: a protocol designed against a real
consumer is better than one designed against an imagined one. See 2.9.

Conceptual shape only, not a wire format. Field names are shown rather than
sample values, because a sample value in a specification becomes a real value in
an implementation.

Flipper to StopBath:

| Verb | Fields | Sent when |
|---|---|---|
| `HELLO` | protocol version, lock state | on attach and on peripheral restart |
| `BUTTON` | event name, foreground flag, lock state | on a transmitted button press |
| `STATE` | lock state | when the screen is locked or unlocked locally |

StopBath to Flipper:

| Verb | Fields | Sent when |
|---|---|---|
| `DISPLAY` | status code, current page, page payload, delivered count, error code | on every change to any field |

One record, not a verb per concern. An earlier draft sketched six display verbs,
which made reconnect a sequence to replay and let the Flipper's view drift field
by field. The record carries the whole visible state and replaces the previous one
outright.

**MUST** send the payload for the current page only. **MUST NOT** send both page
payloads at once, because that would place a Wi-Fi passphrase on the peripheral
while a different page is showing, for no benefit.

The verb set may grow. The control mapping in 2.3 is fixed and any growth must
leave it intact.

**MUST** first determine whether an existing Flipper RPC mechanism already
provides an appropriate contract before defining anything custom. See `FD2`.

**MUST NOT** invent a complicated protocol where a plain serial line is
sufficient.

## 2.9 Building before the appliance side exists

The appliance side is built after this peripheral, not before. That order is
recorded as `PD1` in `STOPBATH_PERIPHERAL_EXTENSION.md` and is the reason this
section exists.

Three consequences, all of which **MUST** be honoured.

### The protocol is drafted here and promoted later

**MUST NOT** make any phase before `FE3` depend on the protocol existing. The
first two phases put a working application on the device with no protocol at all,
which is the point of building the peripheral first. An earlier draft of this
document required the definition to exist in the StopBath repository before `FE1`,
which would have meant designing the appliance API before a single button had been
read on real hardware. That requirement is withdrawn.

**MUST** draft the protocol in this repository during `FE3`, against two phases of
real device behaviour rather than against an imagined appliance.

**MUST** promote the definition to the StopBath repository at the freeze, not
before, and **MUST NOT** treat StopBath as owning it until then.

Promotion is a review, not a file move. A contract drafted in the consumer's
repository acquires the consumer's assumptions, and this is where they get caught.
**MUST** check, field by field, that the appliance can actually supply the value,
that no field exists only because it was convenient to render, and that nothing
encodes a Flipper specific control, display geometry, or key code. Record what
changed at promotion in the change record described below.

### A development peer stands in for the appliance

**MUST** provide a development peer in this repository: a small host program that
speaks the protocol, drives every display state, and records every event
received. (Since 2026-09-16 this directory is `flipper/` in the
`stopbath-peripheral` repository and the peer is held once for both
peripherals under `../shared/peer/`, which is in the repository and satisfies
this rule by its letter; peripheral spec `SD9`.)

**MUST** treat it as a test double, on the same terms as the fakes in the main
specification. It exists so this repository can be developed and tested with no
appliance present.

**MUST NOT** let it become the appliance implementation, be imported by appliance
code, or be the place where interpretation of button events is decided.
Interpretation belongs to StopBath and is specified in the extension document,
not here.

**MUST** make it capable of misbehaving on demand: wrong version, malformed
message, oversized message, silence, disconnection mid-message. A peer that only
behaves correctly tests nothing interesting.

### The protocol is provisional until it is frozen

**MUST** treat the protocol as provisional, and freely revisable, from `FE3`
until the prototype scope in Part 7 is demonstrated against the development peer.

At that point the author accepts it, and from then on rule 0.11 applies in full
and it is a sacred contract. See `FD20`.

**MUST NOT** apply 0.11 before the freeze. Refusing to rename a verb during the
period the design is being discovered is the wrong kind of discipline.

**MUST** record every change made to the protocol before the freeze, with the
reason. That record is the argument for the shape it settles into, and the
extension document's implementation depends on understanding it.

## 2.10 Connection and recovery

Either device may restart independently, and recovery **MUST NOT** require the
photographer to perform repair steps.

On connection or reconnection, in order:

1. the Flipper identifies itself and its supported protocol version
2. StopBath validates compatibility and rejects an incompatible peer explicitly
3. StopBath sends the current authoritative display state
4. the Flipper discards all local state and renders what it was sent

**MUST NOT** reconstruct StopBath state from what the Flipper remembers.

**MUST** indicate clearly when the appliance cannot be reached.

**MUST NOT** queue button events during a disconnection for later replay. A stale
`CENTER_LONG` arriving after reconnection **MUST NOT** be able to terminate a
later session. Discard, do not buffer.

## 2.11 Observability

**MUST** make protocol faults visible rather than silent. Every rejected message,
version mismatch, bound violation, and reconnection **MUST** be counted and
surfaced, on the Flipper as a diagnostic view and on the appliance through the
health and metrics surfaces described in the main specification.

A peripheral that silently stops working is worse than one that plainly reports
that it has.

**MUST NOT** log a payload. Log the verb, the reason category, and a length.

---

# Part 3: Architecture and repository boundary

Two repositories:

```text
StopBath
    main Go application
    domain behaviour
    peripheral protocol definition
    appliance side peripheral implementation

stopbath-flipper
    Flipper application
    SDK integration
    QR rendering
    button input
    protocol client
```

Keeping them separate stops Flipper specific C and build tooling entering the Go
application's build and release lifecycle. Exact naming is `FD13`.

**MUST** add the appliance side to the main specification as ports before any
appliance code is written. The main specification currently has no peripheral
ports, so this is a real gap in that document, not an oversight in this one.
`GuestPhotoSessionCoordinator` **MUST NOT** be reached directly from a peripheral
adapter; the peripheral goes through the application layer like any other
interface.

**MUST NOT** let a peripheral event bypass any validation that the browser
interface performs. The same command, from a different surface, gets the same
checks.

---

# Part 4: Stage one, Plan

**MUST NOT** write implementation code or a behavioural test until the Plan stage
has settled the phase being implemented.

**MUST** produce `docs/evaluation/ACTUAL_CONTRACT_EVALUATION.md` recording, for
every item below: source URL, version or commit or firmware release, retrieval
date, relevant files, observed contract, uncertainties, experiments performed,
conclusions accepted, and decisions still needing the author.

## 4.1 Firmware and SDK

Verify against the actual device and firmware to be supported:

Firmware distribution and version. Application API version and how it is
declared. The build toolchain and how an application is built and installed.
Whether the API is stable across firmware releases, and what breaks when it is
not. Application size and heap limits. Whether an application can be launched
automatically or must be started by hand, which matters for the guard in 2.4.

**MUST** record which firmware distribution is supported. Third party
distributions and the official one do not share an API guarantee, and supporting
several multiplies the maintenance surface. See `FD12`.

## 4.2 Transport

Enumerate what the firmware actually offers, then choose. Candidates to
investigate rather than assume: USB CDC virtual serial claimed by an application,
the existing RPC mechanism, and a GPIO serial connection.

For each, record: whether an application may claim it, whether it survives either
device restarting, how reconnection is detected, throughput and latency,
whether it conflicts with charging or with the device's own USB behaviour, and
how it is inspected while debugging.

**MUST** settle `FD2` before any protocol code is written.

## 4.3 Power

The Flipper connected to the Pi may draw charging current from it.

The main specification already has the guest radio adapter competing for a Pi USB
power budget of roughly 1200 mA shared across everything attached. A charging
Flipper is another claim on the same budget, and the failure mode is a radio that
misbehaves under load rather than an obvious error.

**MUST** measure total draw with the radio adapter and the Flipper both attached
and active, and record it against the budget.

**MUST** raise it in the main specification if the combination does not fit,
rather than solving it silently here.

## 4.4 Display and QR

Measure on the real device: rendering time for a QR at the sizes under
consideration, memory required, contrast and readability in daylight, and
scanning reliability from a representative set of phones at realistic distance
and angle.

**MUST** test with phones that are not the author's.

## 4.5 Plan stage exit criteria

Evidence recorded with exact versions. Firmware distribution and API version
named. Transport selected with reasons. Power measured. QR viability established
by scanning, not by arithmetic. Every **DECIDE** item either answered or
explicitly still open with the phases it blocks.

---

# Stage two, Execute

Each phase **MUST** begin by rereading Part 0, in particular 0.13.

Each phase **MUST** carry all five headings. A phase missing any of them is not
ready to work.

- **Work** included behaviour
- **Excluded** behaviour deliberately not built here, and why
- **Assumptions to verify** beliefs that must be checked before being relied on
- **Tests first** written and failing before implementation
- **Done when** split into automated criteria and, where relevant, a hardware gate

Automated criteria **MUST** be reproducible by the author on a clean machine with
no Flipper attached.

Hardware gate items are cleared by the author on the real device. **MUST NOT**
claim a hardware gate has passed. **MUST NOT** infer one from a green automated
run.

## FE1: Foundation and first light

The point of this phase is to get something running on the device on day one,
with no protocol, no transport, and nothing decided about the appliance.

**Work** Repository structure. Build and install via the SDK toolchain. Warnings
as errors. Em dash and mangled hyphen scan. Continuous integration. An application
that launches on the device, draws to the screen, detects every button in the
table in 2.3, and shows locally which button was pressed. The screen lock from
2.4.

**Precondition** None. **MUST NOT** make this phase depend on the protocol, the
appliance, or any decision in the extension document.

**Excluded** Any protocol, any transport, any appliance concern, any QR, any NFC.
Nothing here talks to anything.

**Assumptions to verify** The application size and heap limits recorded in 4.1.
That the firmware exposes button events and lock state the way 4.1 assumed.

**Tests first** Button to local action mapping. Lock state transitions. Unknown
input is ignored rather than crashing.

**Done when, automated** Builds clean on a machine with no device attached.
Repository checks pass.

**Hardware gate** The application launches on the author's Flipper, draws, and
reports every press. This is the first thing to get working and everything else
waits behind it.

**Tests first** Well formed message parsing for every verb. Truncated message.
Overlong message. Unknown verb. Unknown field. Missing required field. Wrong
version. Embedded null. Non-printable bytes. Maximum length payload exactly at
the bound and one byte over.

## FE2: Application skeleton and display

**Work** Minimal Flipper application. View lifecycle. Static rendering of each
display state from a supplied state structure. Connection status indication.

**Excluded** Transport and protocol wiring, which is FE4. QR rendering, which is
FE5.

**Assumptions to verify** The heap and application size limits recorded in 4.1.

**Tests first** State to layout mapping for every state. Unknown status code
renders a safe fallback rather than blank or garbage. Long values truncate rather
than overflow.

**Done when, automated** Every display state renders from a fixture. No SDK call
result is ignored.

**Hardware gate** Each state is legible on the device in daylight at arm's length.

## FE3: Protocol library and development peer

By this point two phases of real device work have happened, so the protocol is
being written against something that exists rather than something imagined. That
is the whole reason the peripheral is built first.

**Work** Draft the protocol in this repository. The parser and encoder as a
freestanding C library with no SDK dependency, so it builds and tests on a
development machine. Sanitiser build. Fuzz harness for the parser. The development
peer described in 2.9, including its misbehaviour modes.

**Excluded** Wiring the protocol to the transport, which is `FE4`. Any appliance
implementation, which is `PE1` and later in the extension document. Any
interpretation of button meaning, which belongs to the appliance and is never
built here.

**Assumptions to verify** That the protocol library can be built and tested
without the SDK present. That every display state drawn in `FE2` can be expressed
in one state record without a free text field.

**Tests first** Well formed message parsing for every verb. Truncated message.
Overlong message. Unknown verb. Unknown field. Missing required field. Wrong
version. Embedded null. Non-printable bytes. Maximum length payload exactly at the
bound and one byte over.

**Done when, automated** Library builds and tests on a clean machine with no
Flipper and no SDK. Sanitiser build clean. Fuzz harness runs and finds nothing in
a short run. No allocation occurs in the parse path, proven by test. The
development peer drives every display state and records every event, and can
produce each of its misbehaviour modes on request.

## FE4: Transport and session

**Work** The transport selected in `FD2`. Handshake and version negotiation.
Reconnection. State replacement on reconnect. Event transmission. The foreground
and display active guard from 2.4.

**Excluded** QR rendering, which is FE5. Any interpretation of button meaning,
which belongs to the appliance and is never built here.

**Assumptions to verify** That the transport survives either device restarting.
That the application can determine reliably whether it is in the foreground.

**Tests first** Handshake success. Version mismatch rejection. Disconnect during
a message. Reconnect discards local state. Button events are not queued across a
disconnection. An event is not emitted while the guard is unsatisfied.

**Done when, automated** All transport logic that is not SDK bound is covered by
tests on a development machine. Reconnect always results in peer supplied state.

**Hardware gate** Against the development peer, pull and reinsert the cable
twenty times and restart each side independently, without a repair step being
required. The same check against a real appliance belongs to `PE2` in the
extension document and **MUST NOT** be claimed here.

## FE5: QR rendering

**Work** QR encoder integration. Rendering at the chosen module size. Page
switching between supplied payloads.

**Excluded** Deciding what the payloads contain, which belongs to the appliance.

**Assumptions to verify** The rendering time and memory measured in 4.4.

**Tests first** Known payload produces a known matrix, against published test
vectors. Payload too long for the chosen version is refused, not truncated.
Page switching with one, two, and zero supplied payloads.

**Done when, automated** Encoder output matches published vectors. An oversized
payload is refused with a distinct error rather than rendering something
unscannable.

**Hardware gate** Scanning succeeds from a representative set of phones at
realistic distance, angle, and lighting. Record the phones and the conditions.

## FE6: NFC presentation

Only begun once `FD15`, `FD16` and `FD17` are answered. If any answer is
negative, this phase is dropped and the product ships with QR only.

**Work** NDEF record construction for a URI payload and for a Wi-Fi credential
payload. Presenting the target. Deriving both encodings for a page from the one
value supplied by the appliance.

**Excluded** Any NFC reading. The device presents, it does not scan. Any payload
the appliance did not supply.

**Assumptions to verify** The three capability answers, against the real device
rather than against documentation.

**Tests first** URI record construction against published NDEF test vectors.
Wi-Fi credential record construction against a known good reference. Payload too
long for the record is refused rather than truncated. QR and NFC for one page are
derived from a single value and cannot diverge. No payload is retained after the
page changes or the session ends.

**Done when, automated** Record construction matches vectors. Divergence between
the two encodings is impossible by construction, proven by test.

**Hardware gate** An Android phone joins the network by tapping. Both an Android
phone and an iPhone open the gallery by tapping. Record handsets and operating
system versions. Confirm that no payload survives session termination.

## FE7: Field prototype

**Work** Whatever small changes the field questions in Part 8 expose. Nothing
speculative.

**Excluded** Every idea in Part 9. Generalisation of the protocol.

**Tests first** A regression test for each defect found in the field.

**Done when, automated** The suite still passes and each field defect has a test.

**Hardware gate** The first prototype scope in Part 7 is demonstrated end to end.

---

# Stage three, Verify

## V1: Per-phase reproduction report

At the end of every phase **MUST** output: exact commands used, toolchain and SDK
versions, focused test result, full test result, sanitiser result, warning count,
application size against the limit, tests skipped and why, tests requiring author
hardware, and any hardware gate still outstanding.

**MUST NOT** declare a phase complete before the author reproduces the automated
criteria on a clean machine.

## V2: Continuous integration

Every pull request **MUST** run: em dash and mangled hyphen scan; a warnings as
errors build; the protocol library tests; the sanitiser build; a short fuzz run;
and the application build against the pinned SDK.

## V3: Release

**MUST** confirm: every **DECIDE** item resolved or explicitly deferred with the
author's agreement; `IMPLEMENTATION_DEVIATIONS.md` complete; supported firmware
distribution and version named; known limitations published; the appliance
remains fully operable with no Flipper attached.

---

# Part 5: Security

The Flipper is physically connected to an isolated appliance, so the threat model
is narrow. The main specification's threat model and accepted risks remain
authoritative, and this document **MUST NOT** widen them.

**MUST** treat all data received over the peripheral connection as untrusted.

**MUST** bound message sizes and reject malformed messages.

**MUST NOT** allow arbitrary command execution, expose a shell, or accept a
filesystem path from the protocol.

**MUST NOT** send a secret to the Flipper unless it is required for the immediate
interaction. A Wi-Fi passphrase inside a QR payload qualifies. A guest token, a
session identifier, or an administrator credential does not.

**MUST NOT** write a passphrase, token, or session identifier to Flipper storage,
including logs and crash dumps.

**MUST** clear ephemeral sensitive state as soon as it is no longer displayed,
including on session termination and on disconnection.

**MUST NOT** expand the peripheral protocol into a privileged administrative
interface. New verbs are a decision, not an implementation detail.

Worth stating plainly, because it is the honest limit: anyone holding the Flipper
while a session is running can display the QR and terminate the session. The
device is a physical key. That is acceptable for equipment carried by the
photographer and is the same trade the main specification already accepts for the
QR on a phone screen.

---

# Part 6: Testing

Tests cover behaviour, not implementation trivia.

**MUST** automate: protocol parsing, malformed message handling, button event
encoding, QR encoding, state and display command handling, reconnection
behaviour, protocol version handling, and the foreground guard.

**MUST** keep every unit test under one second, including on modest continuous
integration hardware.

**MUST NOT** create a general end to end suite spanning appliance and device. The
substitutes are the protocol library tests, appliance side contract tests against
a fake peripheral, and the physical checks below.

**MUST** record hardware specific behaviour that cannot reasonably be automated as
a testing deviation with the reason, rather than leaving it unstated.

Physical checks, which are product tests and not gaps in coverage: QR scanning
reliability, button ergonomics, long press timing, pocketability, accidental
activation, reconnect behaviour, display readability outdoors, and workflow speed
with real subjects.

---

# Part 7: First prototype scope

The first prototype is brutally small, and it is demonstrated against the
development peer rather than against an appliance, because the appliance side
does not exist yet.

A successful first version:

1. the Flipper connects to a host running the development peer
2. the peer and the application complete a handshake and agree a version
3. the peer sends a short guest URL
4. the Flipper renders a scannable QR
5. left and right switch between two supplied payloads
6. a short centre press produces an event received by the peer
7. a long centre press produces a distinct event received by the peer
8. a short down press locks the screen and no further events are sent
9. a long down press unlocks it
10. the peer sends a status update and the Flipper renders it
11. disconnect and reconnect restores current state safely

Demonstrating the same sequence against a real appliance is the hardware gate of
`PE4` in the extension document. **MUST NOT** treat a green run against the
development peer as evidence about the appliance.

Completing this list is what triggers the protocol freeze in 2.9.

Nothing else is required to prove the concept. NFC is deliberately not in this
list. It is the second thing to prove, not the first, because the QR surface
alone already delivers a working product and NFC depends on three unverified
firmware capabilities.

## Success criteria

The prototype succeeds if it demonstrates that a small dedicated physical
interface materially improves the shooting workflow. Technical completion alone
is insufficient.

The question is whether this lets the photographer stop messing about with a
phone and concentrate on taking photographs. If yes, something valuable has been
established about an eventual physical product. If no, learn why before designing
custom hardware.

---

# Part 8: Field validation

The prototype exists partly to answer questions about a future dedicated
controller. **MUST NOT** answer any of these by design speculation when using the
prototype would answer them.

During real use, record observations on: whether showing the QR from the Flipper
beats using a phone; whether subjects scan it reliably; whether the display is
sufficient; whether two QR pages are understandable; whether left and right is
the right interaction; whether a short centre press for a new session is
intuitive; whether a long centre press for termination is both safe and fast;
whether confirmation is necessary or merely irritating; whether it can be
operated without looking once familiar; whether it survives repeated pocketing;
whether cabling is acceptable; whether wireless would materially improve things;
what status information is actually useful; what turns out to be noise; whether
the photographer still reaches for the dashboard; and whether a dedicated device
would need more buttons or fewer.

---

# Part 9: A future dedicated remote

If the experiment validates the interaction, a later controller might be a
microcontroller with an e-ink display, or a dedicated device, or a third party
implementation of the same protocol.

**MUST NOT** prematurely generalise the protocol because other implementations
might exist one day. Build this one first. Generalise when a second
implementation provides evidence that generalisation is useful. (The second
implementation exists: the Pico remote, `../pico/`, which cleared its
appliance gate on 2026-09-15. The generalisation it justified is not of the
protocol, which the appliance froze unchanged at `FD20`, but of the code:
what both peripherals compile is held once under `../shared/`, per
`../../STOPBATH_PERIPHERAL_SPEC.md`, since 2026-09-16.)

**MUST NOT** bake gratuitous Flipper specifics into the protocol either. The
balance is that the protocol describes a small display, a few buttons, and a
connection, without describing this particular device's model numbers or key
codes.

---

# Part 10: Blocking decisions

**MUST NOT** guess any of these. Ask in the format required by 0.3, batched by
the stage that needs them.

## Before the repository exists

`FD1` **Language and toolchain.** C is presumed because Flipper applications are
written in it. Confirm, and confirm which SDK version and build tool are pinned.

`FD2` **Transport.** Chosen after 4.2. **GUIDANCE** Prefer an existing firmware
mechanism over a custom protocol on a raw serial line, and prefer whichever
survives both devices restarting without manual repair. Do not invent a complex
protocol where a plain serial line suffices.

`FD3` **Display layouts.** Real layouts against the actual display geometry,
replacing the conceptual sketches in 2.6.

## Before QR work

`FD4` **Wi-Fi QR viability.** Whether a Wi-Fi payload with a per-session SSID and
passphrase scans reliably at this display size. If not, whether the Flipper
carries the guest URL only, whether NFC takes the joining role for Android while
iPhone falls back to the phone, or whether the credential grammar in the main
specification changes to fit. The third option is a change to that document and
is raised there.

## Before NFC work

`FD15` **Runtime NDEF emulation.** Whether the firmware allows an application to
present an NDEF target whose payload is chosen at runtime.

`FD16` **Simultaneous QR and NFC.** Whether the screen can show a QR while NFC
emulation is active, or whether they are mutually exclusive. If they are, decide
whether the page alternates, whether a button selects the surface, or whether NFC
is dropped. **GUIDANCE** Do not alternate automatically. A guest holding a phone
against a device whose behaviour changes on a timer is a bad experience, and the
photographer cannot see the screen while presenting it.

`FD17` **Wi-Fi credential record.** Whether the Wi-Fi credential NDEF record can
be emulated in a form Android accepts, on this hardware, with a per-session SSID
and passphrase.

`FD20` **Protocol freeze.** Author acceptance that the protocol has settled,
after the prototype scope in Part 7 is demonstrated. Until this is given, the
protocol is provisional and 0.11 does not apply to it. After it, 0.11 applies in
full and the extension document may be implemented against it.

`FD18` **Guest gallery address.** What the guest URL page actually carries. See
the constraints recorded in the main specification, since a publicly registered
hostname behaves differently in a browser from a local address, and the decision
belongs there rather than here.

## Before destructive actions ship

`FD5` **Whether the guard in 2.4 ever needs more.** Settled for the first version
as no: unlocked, foregrounded, long press. Revisit only if field use produces an
actual accidental termination. Record any that happen, since the origin recording
in 2.4 is what makes them countable.

`FD19` **Screen lock implementation.** Whether the firmware's own device lock can
be entered and left from within the application, or whether an application level
lock is needed. **GUIDANCE** Prefer the firmware's own if it works, since a lock
the user already knows beats one this application invents.

`FD6` **Origin recording.** Whether the dashboard shows the origin of a
termination in normal use or only in the audit record.

## Before the protocol is fixed

`FD7` **Protocol version policy.** Whether an older peer is rejected outright or
negotiated down, and how long an old version stays supported.

`FD8` **Status and error code set.** The enumerated codes, which become sacred
under 0.11 once accepted.

`FD9` **Message bounds.** Maximum message length, maximum payload length, and
maximum queued outbound messages.

## Before hardware work

`FD10` **Power budget.** Whether the Flipper draws charge from the Pi in the
supported configuration, and whether the total with the radio adapter fits. May
force a powered hub, a data only cable, or charging the Flipper separately.

`FD11` **Supported phones for QR testing.** Which handsets constitute a
representative set, given the author cannot test every phone.

`FD12` **Supported firmware distribution.** Official firmware only, or a third
party distribution as well. **GUIDANCE** Official only for the first version.
Supporting several multiplies the API surface that can break underneath the
application, which is the same class of risk as an out of tree driver.

## Housekeeping

`FD13` **Repository name and licence.** **GUIDANCE** Match the main project's
licence so a contributor moving between the two meets one set of terms.

`FD14` **Whether the peripheral ports land in the main specification now or when
appliance work starts.** They are missing from it either way, see Part 3.

---

# Appendix A: Unverified values

None of these is an accepted contract. **MUST** replace each with a measured or
confirmed value, or an explicit author decision, before the phase that depends on
it.

| Value | Starting point | Settled by |
|---|---|---|
| Transport | undecided | `FD2` |
| Protocol version scheme | undecided | `FD7` |
| Event names | the set in 2.3 | `FD8` |
| Status and error codes | undecided | `FD8` |
| Maximum message length | undecided | `FD9` |
| Maximum payload length | undecided | `FD9` |
| QR version and module size | version 1 at two pixels per module, from the arithmetic in 2.7 | 4.4 |
| Guest URL length budget | derived from the QR decision | 4.4 |
| Long press duration | undecided | 4.4 and field use |
| Application heap and size limits | undecided | 4.1 |
| Supported firmware version | undecided | `FD12` |
| Total USB current with radio adapter and Flipper | unmeasured | `FD10` |

# Appendix B: Required repository documents

`README.md`, `PROTOCOL.md` referencing the definition in the main repository,
`TESTING.md`, `IMPLEMENTATION_DEVIATIONS.md`, `HARDWARE_COMPATIBILITY.md`,
`FIELD_NOTES.md`, `docs/evaluation/ACTUAL_CONTRACT_EVALUATION.md`, and a licence
per `FD13`.
