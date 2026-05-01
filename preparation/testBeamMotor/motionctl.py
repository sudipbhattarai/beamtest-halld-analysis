#!/usr/bin/env python3
import sys
import time

import click
import serial

# ── Calibration constants ───────────────────────────────────────────────────────
X_MM_PER_STEP = 0.005  # mm moved per X-step
Y_MM_PER_STEP = 0.005  # mm moved per Y-step
DELTA_Y_STEPS = 400  # steps between Y limit switches


# ── Serial helper functions ────────────────────────────────────────────────────
def open_serial(port: str, baud: int, timeout: float = 1.0):
    """Open and configure the serial port to the Velmex controller."""
    try:
        ser = serial.Serial(port, baudrate=baud, timeout=timeout)
        click.echo(f"🔌  Opened serial port {port} @ {baud} baud")
        return ser
    except serial.SerialException as e:
        click.echo(f"❌  Could not open serial port {port}: {e}", err=True)
        sys.exit(1)


def send_command(ser, cmd: str, read_response: bool = True) -> str:
    """Send a line (adds CR), optionally read one reply line back."""
    ser.write((cmd + "\r").encode())
    # small pause to let controller parse it
    time.sleep(0.05)
    if read_response:
        return ser.readline().decode().strip()
    return ""


# ── Click CLI setup ────────────────────────────────────────────────────────────
@click.group(context_settings={"help_option_names": ["-h", "--help"]})
@click.option(
    "--port",
    "-p",
    default="/dev/ttyUSB0",
    show_default=True,
    help="Serial device for the motor controller",
)
@click.option("--baud", "-b", default=9600, show_default=True, help="Serial baud rate")
@click.pass_context
def cli(ctx, port, baud):
    """
    motorctl — command-line Velmex XY motion controller.
    """
    ctx.obj = {"ser": open_serial(port, baud)}


# ── Move command ───────────────────────────────────────────────────────────────
@cli.command("move")
@click.argument("axis", type=click.Choice(["X", "Y"], case_sensitive=False))
@click.argument("distance", type=float)
@click.option(
    "--absolute",
    "-a",
    is_flag=True,
    default=False,
    help="Perform an absolute move (default is relative)",
)
@click.pass_context
def move(ctx, axis, distance, absolute):
    """
    Move AXIS (X or Y) by DISTANCE in millimeters.
    """
    ser = ctx.obj["ser"]
    mode = "A" if absolute else "D"  # A=absolute, D=delta/relative
    # Convert mm → steps
    mm_per_step = X_MM_PER_STEP if axis.upper() == "X" else Y_MM_PER_STEP
    steps = int(distance / mm_per_step)
    cmd = f"{axis.upper()}{mode}{steps}"
    click.echo(f"🛰  Sending: {cmd}  ({distance:.3f} mm → {steps} steps)")
    resp = send_command(ser, cmd)
    click.echo(f"↩  {resp}")
    # now actually start the motion
    go_cmd = f"{axis.upper()}G"
    send_command(ser, go_cmd, read_response=False)
    click.echo("▶  Motion started")


# ── Home command ───────────────────────────────────────────────────────────────
@cli.command("home")
@click.argument("axis", type=click.Choice(["X", "Y"], case_sensitive=False))
@click.pass_context
def home(ctx, axis):
    """
    Home AXIS until its limit switch is triggered.
    """
    ser = ctx.obj["ser"]
    # Typical Velmex home sequence: zero the encoder then jog toward switch
    # (adjust commands below to your controller’s protocol)
    click.echo(f"🏠  Homing axis {axis.upper()}…")
    resp = send_command(ser, f"{axis.upper()}Z")  # Z = home/zero command?
    click.echo(f"↩  {resp}")
    # start motion
    send_command(ser, f"{axis.upper()}G", read_response=False)
    click.echo("▶  Homing motion started")


# ── Status command ─────────────────────────────────────────────────────────────
@cli.command("status")
@click.pass_context
def status(ctx):
    """
    Query and display controller + axis status.
    """
    ser = ctx.obj["ser"]
    # Again, adjust these to your controller’s status-query commands
    click.echo("🔍  Controller status:")
    for q in ["S?", "X?", "Y?"]:
        resp = send_command(ser, q)
        click.echo(f"  {q} → {resp}")


# ── Calibrate command ──────────────────────────────────────────────────────────
@cli.command("calibrate")
@click.option(
    "--axis",
    "-x",
    "axis",
    type=click.Choice(["X", "Y"], case_sensitive=False),
    required=True,
    help="Axis to calibrate",
)
@click.option(
    "--mm-per-step",
    type=float,
    required=True,
    help="How many mm does one step actually move?",
)
def calibrate(axis, mm_per_step):
    """
    Override the mm/step calibration for AXIS.
    (You’ll need to re-edit the script or persist in a config file to make it permanent.)
    """
    global X_MM_PER_STEP, Y_MM_PER_STEP
    axis = axis.upper()
    if axis == "X":
        X_MM_PER_STEP = mm_per_step
    else:
        Y_MM_PER_STEP = mm_per_step
    click.echo(f"✔  {axis}-axis calibration set to {mm_per_step:.6f} mm/step")


# ── Cleanup on exit ───────────────────────────────────────────────────────────
@cli.resultcallback()
@click.pass_context
def cleanup(ctx, *args, **kwargs):
    ser = ctx.obj.get("ser")
    if ser and ser.is_open:
        ser.close()
        click.echo("🔌  Serial port closed.")


# ── Entry point ────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    cli()
