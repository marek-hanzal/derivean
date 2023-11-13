import {Container} from "@use-pico/container";
import {
    describe,
    expect,
    test
}                  from "vitest";
import {
    type IInventory,
    type IInventoryItem,
    withInventoryContainer,
    withInventoryService
}                  from "../src";

const inventory: IInventory = {
    items: [
        {
            item: {
                name: "tree",
            },
            amount:   5,
        },
        {
            item: {
                name: "tree",
            },
            amount:   7,
        },
        {
            item: {
                name: "log",
            },
            amount:   1,
        },
    ],
};

const normalizedInventory: IInventory = {
    items: [
        {
            item: {
                name: "tree",
            },
            amount:   12,
        },
        {
            item: {
                name: "log",
            },
            amount:   1,
        },
    ],
};

describe("InventoryService", () => {
    const container = new Container();
    withInventoryContainer(container);
    const inventoryService = withInventoryService.use(container);

    test("Normalized inventory", () => {
        expect(inventoryService.normalize(inventory)).toEqual(new Map<string, IInventoryItem>([
            ["tree", {
                item: {name: "tree"},
                amount:   12
            }],
            ["log", {
                item: {name: "log"},
                amount:   1
            }],
        ]));
    });

    test("Normalized inventory back to array", () => {
        expect(inventoryService.normalizeOf(inventory)).toEqual(normalizedInventory);
    });
});
