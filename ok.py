def check_page_allocation_consistency(lines):
    prev_alloc = None
    prev_free = None

    for idx, line in enumerate(lines):
        try:
            parts = line.strip().split(',')
            alloc = int(parts[0].split(':')[1].strip())
            free = int(parts[1].split(':')[1].strip())

            if prev_alloc is not None and prev_free is not None:
                alloc_diff = alloc - prev_alloc
                free_diff = prev_free - free

                if alloc_diff != free_diff:
                    print(f"Inconsistency at line {idx + 1}:")
                    print(f"  Previous -> alloc: {prev_alloc}, free: {prev_free}")
                    print(f"  Current  -> alloc: {alloc}, free: {free}")
                    print(f"  alloc_diff = {alloc_diff}, free_diff = {free_diff}\n")

            prev_alloc = alloc
            prev_free = free

        except (IndexError, ValueError) as e:
            print("skipping")


# Example usage with data from a file
if __name__ == "__main__":
    with open("shit.txt", "r") as f:
        lines = f.readlines()
        check_page_allocation_consistency(lines)

