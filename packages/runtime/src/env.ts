import { CallbackInfoStore } from './CallbackInfo'
import { DeferredStore } from './Deferred'
import { HandleStore } from './Handle'
import { ScopeStore, IHandleScope, HandleScope } from './HandleScope'
import { RefStore } from './Reference'
import { IStoreValue, Store } from './Store'
import { TypedArray, supportFinalizer, TryCatch, envStore, isReferenceType } from './util'

export interface ILastError {
  errorMessage: Int32Array
  engineReserved: Int32Array
  engineErrorCode: Uint32Array
  errorCode: Int32Array
  data: Pointer<napi_extended_error_info>
}

export class Env implements IStoreValue {
  public id: number

  typedArrayMemoryMap = new WeakMap<TypedArray | DataView, void_p>()
  arrayBufferMemoryMap = new WeakMap<ArrayBuffer, void_p>()
  memoryPointerDeleter: FinalizationRegistry<void_p> = supportFinalizer
    ? new FinalizationRegistry<void_p>((heldValue) => {
      this.free(heldValue)
    })
    : null!

  public openHandleScopes: number = 0

  public handleStore!: HandleStore
  public scopeStore!: ScopeStore
  public cbInfoStore!: CallbackInfoStore
  public refStore!: RefStore
  public deferredStore!: DeferredStore

  private _rootScope!: HandleScope
  private currentScope: IHandleScope | null = null

  public tryCatch = new TryCatch()

  public static create (
    malloc: (size: number) => number,
    free: (ptr: number) => void,
    dynCalls: IDynamicCalls,
    Module: any,
    __emnapi_env_new: () => number,
    __emnapi_env_free: (ptr: number) => void,
    _napi_clear_last_error: (env: number) => number,
    _napi_set_last_error: (env: number, code: number, engine_code: number, _: number) => number
  ): Env {
    const env = new Env(malloc, free, dynCalls, Module, __emnapi_env_new, __emnapi_env_free, _napi_clear_last_error, _napi_set_last_error)
    // envStore.add(env)
    const id = __emnapi_env_new()
    envStore.set(id, env)
    env.id = id
    env.refStore = new RefStore()
    env.handleStore = new HandleStore(env)
    env.deferredStore = new DeferredStore()
    env.scopeStore = new ScopeStore()
    env.cbInfoStore = new CallbackInfoStore()

    env._rootScope = HandleScope.create(env, null)
    return env
  }

  private constructor (
    public malloc: (size: number) => number,
    public free: (ptr: number) => void,
    public dynCalls: IDynamicCalls,
    public Module: any,
    public __emnapi_env_new: () => number,
    public __emnapi_env_free: (ptr: number) => void,
    public _napi_clear_last_error: (env: number) => number,
    public _napi_set_last_error: (env: number, code: number, engine_code: number, _: number) => number
  ) {
    this.id = 0
  }

  public openScope<Scope extends HandleScope> (ScopeConstructor = HandleScope): Scope {
    if (this.currentScope) {
      const scope = ScopeConstructor.create(this, this.currentScope)
      this.currentScope.child = scope
      this.currentScope = scope
    } else {
      this.currentScope = this._rootScope
    }
    // this.scopeList.push(scope)
    this.openHandleScopes++
    return this.currentScope as Scope
  }

  public closeScope (scope: IHandleScope): void {
    if (scope === this.currentScope) {
      this.currentScope = scope.parent
    }
    if (scope.parent) {
      scope.parent.child = scope.child
    }
    if (scope.child) {
      scope.child.parent = scope.parent
    }
    if (scope === this._rootScope) {
      scope.clearHandles()
      scope.child = null
    } else {
      scope.dispose()
    }
    // this.scopeList.pop()
    this.openHandleScopes--
  }

  public getCurrentScope (): IHandleScope {
    return this.currentScope!
  }

  public ensureHandleId (value: any): napi_value {
    if (isReferenceType(value)) {
      const handle = this.handleStore.getObjectHandle(value)
      if (!handle) {
        // not exist in handle store
        return this.currentScope!.add(value).id
      }
      if (handle.value === value) {
        // exist in handle store
        return handle.id
      }
      // alive, go back to handle store
      handle.value = value
      Store.prototype.add.call(this.handleStore, handle)
      this.currentScope!.addHandle(handle)
      return handle.id
    }

    return this.currentScope!.add(value).id
  }

  public clearLastError (): napi_status {
    return this._napi_clear_last_error(this.id)
  }

  public setLastError (error_code: napi_status, engine_error_code: uint32_t = 0, engine_reserved: void_p = 0): napi_status {
    return this._napi_set_last_error(this.id, error_code, engine_error_code, engine_reserved)
  }

  public getReturnStatus (): napi_status {
    return !this.tryCatch.hasCaught() ? napi_status.napi_ok : this.setLastError(napi_status.napi_pending_exception)
  }

  public callIntoModule<T> (fn: (env: Env) => T): T {
    this.clearLastError()
    const r = fn(this)
    if (this.tryCatch.hasCaught()) {
      const err = this.tryCatch.extractException()!
      throw err
    }
    return r
  }

  public getViewPointer (view: TypedArray | DataView): void_p {
    if (!supportFinalizer) {
      return NULL
    }
    if (view.buffer === this.Module.HEAPU8.buffer) {
      return view.byteOffset
    }

    let pointer: void_p
    if (this.typedArrayMemoryMap.has(view)) {
      pointer = this.typedArrayMemoryMap.get(view)!
      this.Module.HEAPU8.set(new Uint8Array(view.buffer, view.byteOffset, view.byteLength), pointer)
      return pointer
    }

    pointer = this.malloc(view.byteLength)
    this.Module.HEAPU8.set(new Uint8Array(view.buffer, view.byteOffset, view.byteLength), pointer)
    this.typedArrayMemoryMap.set(view, pointer)
    this.memoryPointerDeleter.register(view, pointer)
    return pointer
  }

  public getArrayBufferPointer (arrayBuffer: ArrayBuffer): void_p {
    if ((!supportFinalizer) || (arrayBuffer === this.Module.HEAPU8.buffer)) {
      return NULL
    }

    let pointer: void_p
    if (this.arrayBufferMemoryMap.has(arrayBuffer)) {
      pointer = this.arrayBufferMemoryMap.get(arrayBuffer)!
      this.Module.HEAPU8.set(new Uint8Array(arrayBuffer), pointer)
      return pointer
    }

    pointer = this.malloc(arrayBuffer.byteLength)
    this.Module.HEAPU8.set(new Uint8Array(arrayBuffer), pointer)
    this.arrayBufferMemoryMap.set(arrayBuffer, pointer)
    this.memoryPointerDeleter.register(arrayBuffer, pointer)
    return pointer
  }

  public dispose (): void {
    // this.scopeList.clear()
    this.currentScope = null
    this.deferredStore.dispose()
    this.refStore.dispose()
    this.scopeStore.dispose()
    this.handleStore.dispose()
    this.tryCatch.extractException()
    this.__emnapi_env_free(this.id)
    envStore.delete(this.id)
  }
}
