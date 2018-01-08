import sys

outputFile = open("output.txt", "w")

bracket_index = 0
masked = False
masked_caller = ""
masked_filename = ""

class LogBP(gdb.Breakpoint):
    def stop(self):
        global bracket_index, masked, masked_caller, masked_filename
        caller = gdb.newest_frame().older().older().name()
        caller_filename = gdb.newest_frame().older().older().function().symtab.filename
        msg, is_field = gdb.lookup_symbol("msg")
        if masked and caller_filename != masked_filename and caller != "print_task":
            #gdb.write("Masking log call from %s during to an unfinished JSON object output made by %s\n" % (caller, masked_caller))
            pass
        else:
            if msg is not None:
                try:
                    outputString = msg.value(gdb.newest_frame()).string()
                    for c in outputString:
                        if(c=="{"):
                            bracket_index += 1
                        elif(c=="}"):
                            bracket_index -= 1

                    outputFile.write(msg.value(gdb.newest_frame()).string())
                    if(bracket_index != 0):
                        if masked == False:
                            masked = True
                            masked_caller = caller
                            masked_filename = caller_filename
                    else:
                        masked = False

                except Exception as e:
                    gdb.write("Error detected, caller: %s\n" % (caller))
                    print(e)
            else:
                gdb.write("Error: Symbol msg not found\n")

logBP = LogBP("gdb_log", gdb.BP_BREAKPOINT)

def exit_handler(event):
    gdb.write("Exited. output.txt saved.")
    outputFile.close()

gdb.events.exited.connect(exit_handler)

for bp in gdb.breakpoints():
    gdb.write(bp.location)
    gdb.write("\n")

