class Rectangle:
    def __init__(self, x, y, width, height):
        self.x = x
        self.y = y
        self.width = width
        self.height = height

    def collides_with(self, other):
        return not (self.x + self.width <= other.x or
                    self.x >= other.x + other.width or
                    self.y + self.height <= other.y or
                    self.y >= other.y + other.height)

    def split(self, other):
        if not self.collides_with(other):
            return [self]
        
        new_rects = []
        if other.x > self.x:
            new_rects.append(Rectangle(self.x, self.y, other.x - self.x, self.height))
        if other.y > self.y:
            new_rects.append(Rectangle(max(self.x, other.x), self.y, min(self.x + self.width, other.x + other.width) - max(self.x, other.x), other.y - self.y))
        if other.x + other.width < self.x + self.width:
            new_rects.append(Rectangle(other.x + other.width, self.y, self.x + self.width - (other.x + other.width), self.height))
        if other.y + other.height < self.y + self.height:
            new_rects.append(Rectangle(max(self.x, other.x), other.y + other.height, min(self.x + self.width, other.x + other.width) - max(self.x, other.x), self.y + self.height - (other.y + other.height)))
        
        return new_rects


# Create two rectangles
rect1 = Rectangle(0, 0, 10, 10)
rect2 = Rectangle(0, 0, 5, 5)

# Check if they collide
print(rect1.collides_with(rect2))  # Output: True

# Split the first rectangle
split_rects = rect1.split(rect2)

# Print the new rectangles
for rect in split_rects:
    print(f"Rectangle at ({rect.x}, {rect.y}) with width {rect.width} and height {rect.height}")