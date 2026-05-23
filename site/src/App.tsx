import { useState, useEffect } from 'react'
import reactLogo from './assets/react.svg'
import viteLogo from '/vite.svg'
import './App.css'

function App() {
  const [message, setMessage] = useState("")
  const [ledOn, setLedOn] = useState(false)

  const getHelloWorld = async () => {
    const webResult = await fetch("/api/hello-world")  // server proxy can cut "http://http://porta1-v01.local"
    const myText = await webResult.text()
    setMessage(myText);
  }
 
  const switchLed = async (is_on: boolean) => {
    const payload = { is_on }
    const webResult = await fetch("/api/toggle-led", {
      method: "POST",
      body: JSON.stringify(payload)
    }) 
    if(!webResult.ok){
      console.error(webResult.statusText)
      return;
    }
    setLedOn(is_on);
  }

  const getLEDText = () => ledOn ? "LED is on" : "LED is off"

  useEffect(() => {
    getHelloWorld();
  }, []) // [] mean run this function only the first time that app renders
 
  return (
    <>
      <div>
        <a href="https://vite.dev" target="_blank">
          <img src={viteLogo} className="logo" alt="Vite logo" />
        </a>
        <a href="https://react.dev" target="_blank">
          <img src={reactLogo} className="logo react" alt="React logo" />
        </a>
      </div>
      <h1>{message}</h1>
      <div className="card">
        <button 
          style={{background: ledOn ? "lightBlue" : ""}} 
          onClick={() => switchLed(!ledOn)}
        >
          {getLEDText()}
        </button>
        <p>
          Edit <code>src/App.tsx</code> and save to test HMR
        </p>
      </div>
      <p className="read-the-docs">
        Click on the Vite and React logos to learn more
      </p>
    </>
  )
}

export default App
