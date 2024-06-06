class Rectangle:
    def __init__(self, x, y, width, height, writing_dir="ltr"):
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.writing_dir = writing_dir
        self.next = None

    def __repr__(self) -> str:
        return f"Rectangle at ({self.x}, {self.y}) with width {self.width} and height {self.height}"

    def collides_with(self, other):
        return (self.x + self.width >= other.x or
                    self.x <= other.x + other.width or
                    self.y + self.height >= other.y or
                    self.y <= other.y + other.height)
    
    def area(self):
        return self.width * self.height

    def split(self, other):
        if not self.collides_with(other):
            return [self]

        top = Rectangle(
            self.x,
            self.y,
            self.width,
            other.y
        )
        left = Rectangle(
            self.x,
            other.y,
            other.x,
            self.height - other.y
        )
        right = Rectangle(
            other.x + other.width,
            other.y,
            self.width - other.x - other.width,
            self.height - other.y
        )
        bottom = Rectangle(
            self.x,
            other.y + other.height,
            self.width,
            self.height - (other.y + other.height)
        )
        if self.writing_dir == "ltr":
            return [rect for rect in [top, left, right, bottom] if rect.area() > 0]
        return [rect for rect in [top, right, left, bottom] if rect.area() > 0]
        


# Create two rectangles
rect1 = Rectangle(0, 0, 10, 10)
rect2 = Rectangle(3, 3, 12, 5)

# Check if they collide
print(rect1.collides_with(rect2))  # Output: True

# Split the first rectangle
split_rects = rect1.split(rect2)

# Print the new rectangles
for rect in split_rects:
    print(f"Rectangle at ({rect.x}, {rect.y}) with width {rect.width} and height {rect.height}")