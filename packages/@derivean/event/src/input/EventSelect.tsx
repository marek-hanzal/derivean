import {
    type ValuesSchema,
    withSourceQueryInput
}                            from "@use-pico/form";
import {
    type ComponentProps,
    useCallback
}                            from "react";
import {EventInline}         from "../inline/EventInline";
import {EventQueryStore}     from "../query/EventQueryStore";
import {EventRpc}            from "../rpc/EventRpc";
import {EventSelectionStore} from "../store/EventSelectionStore";
import {EventTable}          from "../table/EventTable";

const EventQueryInput = withSourceQueryInput({
    withQueryStore:  EventQueryStore,
    withSourceQuery: EventRpc.query,
    SelectionStore:  EventSelectionStore.single,
});
type EventQueryInput<
    TValuesSchema extends ValuesSchema,
> = typeof EventQueryInput<TValuesSchema>;

export namespace EventSelect {
    export interface Props<
        TValuesSchema extends ValuesSchema,
    > extends Omit<
        ComponentProps<EventQueryInput<TValuesSchema>>,
        "Selector" | "Item"
    > {
        tableProps?: Partial<ComponentProps<typeof EventTable>>;
    }
}

export const EventSelect = <
    TValuesSchema extends ValuesSchema,
>(
    {
        tableProps,
        ...props
    }: EventSelect.Props<TValuesSchema>
) => {
    return <EventQueryInput
        Selector={useCallback(() => <EventTable withLinkLock {...tableProps}/>, [])}
        Item={EventInline}
        {...props}
    />;
};
