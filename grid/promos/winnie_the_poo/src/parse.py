from bs4 import BeautifulSoup


with open("pg67098-images.html", "r") as doc:
    text = doc.read()
    soup = BeautifulSoup(text, "html.parser")
    res = []
    for para in soup.find_all("p"):
        new_text = para.text.replace("\n", " ")
        res.append(f'<para style="p">{new_text}</para>')
    print("\n".join(res))