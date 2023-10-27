import {Container} from "@use-pico/container";
import {
    describe,
    expect,
    test
}                  from "vitest";
import {
    InventoryResourceSchema,
    InventorySchema,
    withInventoryContainer,
    withInventoryService
}                  from "../src";

const inventory: InventorySchema.Type = {
    resources: [
        {
            resource: {
                name: "tree",
            },
            amount:   5,
        },
        {
            resource: {
                name: "tree",
            },
            amount:   7,
        },
        {
            resource: {
                name: "log",
            },
            amount:   1,
        },
    ],
};

const normalizedInventory: InventorySchema.Type = {
    resources: [
        {
            resource: {
                name: "tree",
            },
            amount:   12,
        },
        {
            resource: {
                name: "log",
            },
            amount:   1,
        },
    ],
};

describe("InventoryService", () => {
    const container = new Container();
    withInventoryContainer(container);
    const inventoryService = withInventoryService.resolve(container);

    test("Normalized inventory", () => {
        expect(inventoryService.normalize(inventory)).toEqual(new Map<string, InventoryResourceSchema.Type>([
            ["tree", {resource: {name: "tree"}, amount: 12}],
            ["log", {resource: {name: "log"}, amount: 1}],
        ]));
    });

    test("Normalized inventory back to array", () => {
        expect(inventoryService.normalizeOf(inventory)).toEqual(normalizedInventory);
    });
});
