import {
    type ValuesSchema,
    withSourceQueryInput
}                                from "@use-pico/form";
import {
    type ComponentProps,
    useCallback
}                                from "react";
import {InventoryInline}         from "../inline/InventoryInline";
import {InventoryQueryStore}     from "../query/InventoryQueryStore";
import {InventoryRpc}            from "../rpc/InventoryRpc";
import {InventorySelectionStore} from "../store/InventorySelectionStore";
import {InventoryTable}          from "../table/InventoryTable";

const InventoryQueryInput = withSourceQueryInput({
    withQueryStore:  InventoryQueryStore,
    withSourceQuery: InventoryRpc.query,
    SelectionStore:  InventorySelectionStore.single,
});
type InventoryQueryInput<
    TValuesSchema extends ValuesSchema,
> = typeof InventoryQueryInput<TValuesSchema>;

export namespace InventorySelect {
    export interface Props<
        TValuesSchema extends ValuesSchema,
    > extends Omit<
        ComponentProps<InventoryQueryInput<TValuesSchema>>,
        "Selector" | "Item"
    > {
        tableProps?: Partial<ComponentProps<typeof InventoryTable>>;
    }
}

export const InventorySelect = <
    TValuesSchema extends ValuesSchema,
>(
    {
        tableProps,
        ...props
    }: InventorySelect.Props<TValuesSchema>
) => {
    return <InventoryQueryInput
        Selector={useCallback(() => <InventoryTable withLinkLock {...tableProps}/>, [])}
        Item={InventoryInline}
        {...props}
    />;
};
