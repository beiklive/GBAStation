import json



def extract_nearest_quoted(line, keyword):

    pos = line.find(keyword)

    if pos == -1:
        return ""

    # 从 keyword 后开始找 "
    first = line.find('"', pos + len(keyword))

    if first == -1:
        return ""

    second = line.find('"', first + 1)

    if second == -1:
        return ""

    return line[first + 1:second]



if __name__ == "__main__":

    input_dat = [
        "Nintendo - Game Boy Advance.dat",
        "Nintendo - Game Boy Color.dat",
        "Nintendo - Game Boy.dat"
    ]

    output_json = [
        "gba_db.json",
        "gbc_db.json",
        "gb_db.json"
    ]

    for i in range(len(input_dat)):

        result = []

        with open(input_dat[i], "r", encoding="utf-8") as f:

            inside_game = False

            game_name = ""
            game_serial = ""
            game_crc = ""

            for raw_line in f:

                line = raw_line.strip()

                # game start
                if line.startswith("game ("):

                    inside_game = True

                    game_name = ""
                    game_serial = ""
                    game_crc = ""


                if not inside_game:
                    continue

                # game name
                if line.startswith("name "):

                    if not game_name:

                        game_name = extract_nearest_quoted(
                            line,
                            "name"
                        )


                # serial
                if "serial" in line:

                    serial1 = extract_nearest_quoted(
                        line,
                        "serial"
                    )
                    if serial1:
                        game_serial = serial1.upper()


                # crc
                if "crc " in line:

                    pos = line.find("crc ")

                    if pos != -1:

                        crc_part = line[pos + 4:]

                        game_crc = crc_part.split()[0].upper()


                # game end
                if line == ")":

                    if game_crc and game_name:

                        result.append({
                            "crc32": game_crc,
                            "serial": game_serial,
                            "name": game_name
                        })

                    inside_game = False

        with open(output_json[i], "w", encoding="utf-8") as out:

            json.dump(
                result,
                out,
                ensure_ascii=False,
                indent=4
            )

        print(f"{output_json[i]} Done")
