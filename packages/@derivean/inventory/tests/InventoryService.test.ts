import {Container} from "@use-pico/container";
import {
    describe,
    expect,
    test
}                  from "vitest";
import {
    type IInventory,
    type IInventoryResource,
    withInventoryContainer,
    withInventoryService
}                  from "../src";

const inventory: IInventory = {
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

const normalizedInventory: IInventory = {
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
    const inventoryService = withInventoryService.use(container);

    test("Normalized inventory", () => {
        expect(inventoryService.normalize(inventory)).toEqual(new Map<string, IInventoryResource>([
            ["tree", {
                resource: {name: "tree"},
                amount:   12
            }],
            ["log", {
                resource: {name: "log"},
                amount:   1
            }],
        ]));
    });

    test("Normalized inventory back to array", () => {
        expect(inventoryService.normalizeOf(inventory)).toEqual(normalizedInventory);
    });
});
