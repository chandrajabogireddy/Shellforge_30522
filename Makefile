CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
LIBS = -lreadline

# -----------------------------
# Milestone 1
# -----------------------------
M1_SRC = src/history.c

# -----------------------------
# Milestone 2
# -----------------------------
M2_SRC = src/token.c \
         src/lexer.c \
         src/parser.c \
         src/expand.c \
         src/builtin.c \
         src/history.c

# -----------------------------
# Milestone 3
# -----------------------------
M3_SRC = $(M2_SRC) \
         src/executer.c

# -----------------------------
# Milestone 4
# Background Job Management
# -----------------------------
M4_SRC = $(M3_SRC) \
         src/jobs.c

# -----------------------------
# Milestone 5
# Job Control
# -----------------------------
M5_SRC = $(M4_SRC) \
         src/job_control.c

# -----------------------------
# Final ShellForge
# -----------------------------
FINAL_SRC = $(M5_SRC)


.PHONY: all milestone1 milestone2 milestone3 milestone4 milestone5 shellforge clean


all: milestone1 milestone2 milestone3 milestone4 milestone5 shellforge


# Milestone 1
milestone1:
	$(CC) $(CFLAGS) $(M1_SRC) src/main_m1.c $(LIBS) -o milestone1


# Milestone 2
milestone2:
	$(CC) $(CFLAGS) $(M2_SRC) src/main_m2.c $(LIBS) -o milestone2


# Milestone 3
milestone3:
	$(CC) $(CFLAGS) $(M3_SRC) src/main_m3.c $(LIBS) -o milestone3


# Milestone 4
milestone4:
	$(CC) $(CFLAGS) $(M4_SRC) src/main_m4.c $(LIBS) -o milestone4

# Milestone 5
milestone5:
	$(CC) $(CFLAGS) $(M5_SRC) src/main_m5.c $(LIBS) -o milestone5

# Final ShellForge
shellforge:
	$(CC) $(CFLAGS) $(M2_SRC) src/executer_final.c src/jobs.c src/job_control.c src/main.c $(LIBS) -o shellforge



clean:
	rm -f milestone1 milestone2 milestone3 milestone4 milestone5 shellforge
