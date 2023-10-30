"use client";

import {useSuccessNotification} from "@use-pico2/hook";
import {Translation}            from "@use-pico2/i18n";
import {
    type FilterSchema,
    type IWithMutation,
    type QuerySchema,
    useMutation
}                               from "@use-pico2/query";
import {
    type IWithSourceQuery,
    type MutationSchema
}                               from "@use-pico2/source";
import {
    Button,
    CloseIcon,
    Divider,
    Flex,
    Modal,
    ModalStore,
    TrashIcon
}                               from "@use-pico2/ui";

export namespace DeleteByModal {
    export interface Props<
        TFilterSchema extends FilterSchema,
    > extends Modal.Props {
        withMutation: IWithMutation<
            MutationSchema<any, QuerySchema<TFilterSchema, any>>,
            any
        >;
        withSourceQuery: IWithSourceQuery<QuerySchema<TFilterSchema, any>, any>;
    }
}

export const DeleteByModal = <
    TFilterSchema extends FilterSchema,
>(
    {
        withMutation,
        withSourceQuery,
        ...props
    }: DeleteByModal.Props<TFilterSchema>
) => {
    const {close} = ModalStore.use(({close}) => ({close}));
    const successNotification = useSuccessNotification();
    const {
        where,
        filter,
    } = withSourceQuery.store.use((
        {
            where,
            filter
        }) => ({
        where,
        filter
    }));
    const deleteMutation = useMutation({withMutation});

    return <Modal
        modalProps={{
            closeOnClickOutside: !deleteMutation.isPending,
        }}
        title={"delete-by.title"}
        {...props}
    >
        <Translation withLabel={"delete-by.content"}/>
        <Divider mt={"sm"} mb={"sm"}/>
        <Flex
            justify={"space-between"}
            align={"center"}
        >
            <Button
                leftSection={<CloseIcon/>}
                size={"md"}
                color={"blue"}
                variant={"subtle"}
                disabled={deleteMutation.isPending}
                onClick={() => close(props.modalId)}
            >
                <Translation namespace={"common"} withLabel={"cancel.button"}/>
            </Button>
            <Button
                size={"lg"}
                color={"red"}
                leftSection={<TrashIcon/>}
                loading={deleteMutation.isPending}
                onClick={() => {
                    deleteMutation.mutate({
                        delete: {
                            filter,
                            where,
                        },
                    }, {
                        onSuccess: response => {
                            successNotification({
                                withTranslation: {
                                    label:  "delete",
                                    values: response,
                                },
                            });
                        },
                        onSettled: () => close(props.modalId),
                    });
                }}
            >
                <Translation withLabel={"delete-by.confirm.button"}/>
            </Button>
        </Flex>
    </Modal>;
};
