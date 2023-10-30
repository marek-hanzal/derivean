"use client";

import {useSuccessNotification} from "@use-pico2/hook";
import {Translation}            from "@use-pico2/i18n";
import {
    type IWithMutation,
    useMutation
}                               from "@use-pico2/query";
import {type ResponseSchema}    from "@use-pico2/schema";
import {type MutationSchema}    from "@use-pico2/source";
import {type WithEntity}        from "@use-pico2/types";
import {
    Button,
    CloseIcon,
    Divider,
    Flex,
    Modal,
    ModalStore,
    TrashIcon
}                               from "@use-pico2/ui";

export namespace DeleteModal {
    export interface Props<
        TMutationSchema extends MutationSchema<any, any>,
        TResponseSchema extends ResponseSchema,
    > extends Modal.Props, WithEntity.Schema<TResponseSchema> {
        withMutation: IWithMutation<
            TMutationSchema,
            TResponseSchema
        >;
    }
}

export const DeleteModal = <
    TMutationSchema extends MutationSchema<any, any>,
    TResponseSchema extends ResponseSchema,
>(
    {
        withMutation,
        entity,
        ...props
    }: DeleteModal.Props<TMutationSchema, TResponseSchema>
) => {
    const {close} = ModalStore.use(({close}) => ({close}));
    const successNotification = useSuccessNotification();
    const deleteMutation = useMutation({withMutation});

    return <Modal
        modalProps={{
            closeOnClickOutside: !deleteMutation.isPending,
        }}
        title={"delete.title"}
        {...props}
    >
        <Translation withLabel={"delete.content"} values={entity}/>
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
                disabled={deleteMutation.isPending || deleteMutation.isSuccess}
                onClick={() => close(props.modalId)}
            >
                <Translation namespace={"common"} withLabel={"cancel.button"}/>
            </Button>
            <Button
                size={"lg"}
                color={"red"}
                leftSection={<TrashIcon/>}
                loading={deleteMutation.isPending || deleteMutation.isSuccess}
                onClick={() => {
                    deleteMutation.mutate({
                        delete: {
                            where: {
                                id: entity.id,
                            },
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
                <Translation withLabel={"delete.confirm.button"}/>
            </Button>
        </Flex>
    </Modal>;
};