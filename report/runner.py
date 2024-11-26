import subprocess
import re

# Function to run the binary and capture output
def run_binary(binary_path, args: list[str]) -> str:
    try:
        print(f"rdt.exe {" ".join(args)}")
        result = subprocess.run(
            [binary_path] + args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        if result.returncode != 0:
            raise RuntimeError(f"Binary failed with error: {result.stderr}")
        return result.stdout
    except Exception as e:
        print(f"Error running binary: {e}")
        return ""


def percent_change(new: float, old: float) -> float:
    if old == 0:
        old += 0.00001
    return (new - old) / old * 100


# Function to parse output data
def parse_steady_state_values(output) -> tuple[float, float]:
    lines = output.splitlines()

    prev_speed: float = 0
    prev_rtt: float = 0

    final_speed: float = 0
    final_rtt: float = 0

    for line in lines:
        match: re.Match[str] | None = re.match(r"\[.*\].*S\s+([\d.]+).*Mbps.*RTT\s*([\d.]+)", line)
        if match:
            speed = float(match.group(1))
            rtt = float(match.group(2))

            # print("Window: %.3f (%.2f%%) \t\b\b RTT: %.3f (%.2f%%)" % (speed, percent_change(speed, prev_speed), rtt, percent_change(rtt, prev_rtt)))

            if abs(percent_change(speed, prev_speed)) < 0.25 and final_speed == 0:
                final_speed = speed

            if abs(percent_change(rtt, prev_rtt)) < 0.25 and final_rtt == 0:
                final_rtt = rtt

            if( final_speed != 0) and (final_rtt != 0):
                break

            prev_speed = speed
            prev_rtt = rtt

    return (final_speed, final_rtt)


def write_to_csv(filename: str, header: str, data: list[tuple]):
    with open(filename, "w") as csv:
        csv.write(header)
        csv.write("\n".join([f"{x},{y}" for (x, y) in data]))

def q2(exe_path: str):
    data: list[tuple[int, float]] = []
    for i in range(0,10):
        # print(f"{(10 * 2 ** i) / 1000} s")
        # continue
        output: str = run_binary(exe_path, f"localhost {24 - i} 30 {(10 * 2 ** i) / 1000} 0 0 1000".split(" "))
        (speed, rtt) = parse_steady_state_values(output)
        print(f"{rtt}: {speed}")
        data.append((rtt, speed))
    
    write_to_csv("./report/question-2.csv", "rtt,speed (Mbps)\n", data)

def q1(exe_path: str):
    data: list[tuple[int, float]] = []
    for i in range(0, 11):
        # continue
        output: str = run_binary(exe_path, f"localhost {14 + i} {2 ** i} 0.5 0 0 1000".split(" "))
        (speed, rtt) = parse_steady_state_values(output)
        print(f"{2 ** i}: {speed}")
        data.append((2 ** i, speed))
    
    write_to_csv("./report/question-1.csv", "window size,speed (Mbps)\n", data)

# Main function to orchestrate everything
def main():
    binary_path = "C:\\Users\\colem\\OneDrive\\Documents\\School\\(5) Senior Year - Fall\\CSCE463\\hw3\\x64\\Release\\rdt.exe"  # Replace with your binary's path

    # q2(binary_path)
    q1(binary_path)
    

if __name__ == "__main__":
    main()
